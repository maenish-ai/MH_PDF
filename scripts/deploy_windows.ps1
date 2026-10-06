param(
    [Parameter(Mandatory = $true)][string]$QtRootDir,
    [Parameter(Mandatory = $true)][string]$BuildExe,
    [string]$DistDir = "dist"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$distPath = [IO.Path]::GetFullPath((Join-Path $PWD $DistDir))
$exePath = [IO.Path]::GetFullPath((Join-Path $PWD $BuildExe))

if (!(Test-Path $exePath)) {
    throw "Built executable was not found: $exePath"
}

New-Item -ItemType Directory -Force $distPath | Out-Null
Copy-Item $exePath (Join-Path $distPath "MaenPDF.exe") -Force

$windeployqt = Join-Path $QtRootDir "bin\windeployqt.exe"
if (!(Test-Path $windeployqt)) {
    throw "windeployqt.exe was not found: $windeployqt"
}

# Qt deployment is intentionally separate from the MSVC runtime deployment.
# GitHub's Visual Studio generator can build successfully without exporting
# VCINSTALLDIR into the PowerShell environment, which makes
# windeployqt --compiler-runtime unreliable on hosted runners.
& $windeployqt `
    --release `
    --force `
    --qmldir (Join-Path $PWD "ui") `
    --no-translations `
    (Join-Path $distPath "MaenPDF.exe")
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

# Bundle the complete VC143 CRT side-by-side with the application. This keeps
# MaenPDF self-contained on supported Windows 10/11 systems, including PCs
# that do not already have the Visual C++ 2015-2022 Redistributable installed.
$crtSource = $null
$programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
$vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsInstall = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
    if ($vsInstall) {
        $redistRoot = Join-Path $vsInstall "VC\Redist\MSVC"
        if (Test-Path $redistRoot) {
            $crtSource = Get-ChildItem $redistRoot -Directory |
                Sort-Object Name -Descending |
                ForEach-Object { Join-Path $_.FullName "x64\Microsoft.VC143.CRT" } |
                Where-Object { Test-Path $_ } |
                Select-Object -First 1
        }
    }
}

if ($crtSource) {
    Write-Host "Bundling VC143 CRT from: $crtSource"
    Copy-Item (Join-Path $crtSource "*.dll") $distPath -Force
} else {
    # Fallback for runner image/layout changes. Copy the same redistributable
    # CRT family from System32 if the Visual Studio redist folder cannot be
    # located. Windows system DLLs such as ucrtbase.dll are intentionally not
    # copied; supported Windows 10/11 include the Universal CRT.
    Write-Warning "VC143 redist directory was not found through vswhere; using System32 CRT fallback."
    $patterns = @("msvcp140*.dll", "vcruntime140*.dll", "concrt140.dll")
    foreach ($pattern in $patterns) {
        $matches = Get-ChildItem (Join-Path $env:WINDIR "System32\$pattern") -ErrorAction SilentlyContinue
        foreach ($match in $matches) {
            Copy-Item $match.FullName $distPath -Force
        }
    }
}

$required = @(
    "platforms\qwindows.dll",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Qml.dll",
    "Qt6Quick.dll",
    "Qt6QuickControls2.dll",
    "Qt6Pdf.dll",
    "Qt6Widgets.dll",
    "Qt6PrintSupport.dll",
    "msvcp140.dll",
    "vcruntime140.dll",
    "vcruntime140_1.dll"
)
foreach ($relative in $required) {
    $candidate = Join-Path $distPath $relative
    if (!(Test-Path $candidate)) {
        throw "Required Windows runtime is missing: $relative"
    }
}

$crtFiles = Get-ChildItem $distPath -File | Where-Object {
    $_.Name -like "msvcp140*.dll" -or
    $_.Name -like "vcruntime140*.dll" -or
    $_.Name -eq "concrt140.dll"
}
if ($crtFiles.Count -lt 3) {
    throw "MSVC runtime deployment is incomplete; only $($crtFiles.Count) CRT files were bundled."
}

$inventory = Join-Path $distPath "windows-runtime-inventory.txt"
@(
    "MaenPDF Windows runtime inventory",
    "QtRoot=$QtRootDir",
    "VC143Source=$crtSource",
    "GeneratedUtc=$([DateTime]::UtcNow.ToString('o'))",
    ""
) | Set-Content $inventory -Encoding UTF8
Get-ChildItem $distPath -File | Sort-Object Name | ForEach-Object {
    "{0}`t{1}" -f $_.Name, $_.Length
} | Add-Content $inventory -Encoding UTF8

Write-Host "Windows runtime deployment complete."
Get-Content $inventory | Write-Host
