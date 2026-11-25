param([Parameter(Mandatory = $true)][string]$InstalledExe)
$ErrorActionPreference = 'Stop'
$InstalledExe = (Resolve-Path -LiteralPath $InstalledExe).Path

$fixtureDir = Join-Path $env:RUNNER_TEMP ([Guid]::NewGuid().ToString())
New-Item -ItemType Directory -Path $fixtureDir | Out-Null
$log = Join-Path $env:LOCALAPPDATA 'MaenPDF\MaenPDF\logs\maenpdf.log'
$oldPath = $env:PATH
try {
    # Do not let MSYS2 or system-wide OCR installations supply missing files.
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $inputPdf = Join-Path $fixtureDir 'scan.pdf'
    $outputPdf = Join-Path $fixtureDir 'searchable.pdf'
    $arguments = @('--ocr-smoke', ('"{0}"' -f $inputPdf), ('"{0}"' -f $outputPdf))
    $process = Start-Process -FilePath $InstalledExe -ArgumentList $arguments -PassThru
    if (!$process.WaitForExit(120000)) {
        Stop-Process -Id $process.Id -Force
        throw 'Bundled OCR timed out'
    }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $outputPdf)) {
        if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log -Tail 30 }
        throw "Bundled bilingual OCR failed with exit code $($process.ExitCode)"
    }
} finally {
    $env:PATH = $oldPath
    Remove-Item -LiteralPath $fixtureDir -Recurse -Force
}
