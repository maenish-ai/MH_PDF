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
    $appDir = Split-Path -Parent $InstalledExe
    $tesseract = Join-Path $appDir 'tools/tesseract/tesseract.exe'
    $qpdf = Join-Path $appDir 'tools/qpdf/qpdf.exe'
    $tessdata = Join-Path $appDir 'tools/tesseract/tessdata'
    $languages = & $tesseract --list-langs --tessdata-dir $tessdata
    if ($LASTEXITCODE -ne 0 -or $languages -notcontains 'eng' -or $languages -notcontains 'ara') {
        throw "Bundled Tesseract could not load Arabic and English (exit $LASTEXITCODE)"
    }
    & $qpdf --version
    if ($LASTEXITCODE -ne 0) { throw "Bundled qpdf could not start (exit $LASTEXITCODE)" }
    # Keep the Windows app's normal environment. Direct checks above prove
    # the provider executables can start with only the system PATH.
    $env:PATH = $oldPath
    $inputPdf = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../tests/fixtures/ocr-bilingual-scan.pdf')).Path
    $outputPdf = Join-Path $fixtureDir 'searchable.pdf'
    $arguments = @('--ocr-smoke', ('"{0}"' -f $inputPdf), ('"{0}"' -f $outputPdf))
    $stdout = Join-Path $fixtureDir 'stdout.log'
    $stderr = Join-Path $fixtureDir 'stderr.log'
    $process = Start-Process -FilePath $InstalledExe -ArgumentList $arguments -PassThru `
        -WorkingDirectory $appDir -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    if (!$process.WaitForExit(120000)) {
        Stop-Process -Id $process.Id -Force
        throw 'Bundled OCR timed out'
    }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $outputPdf)) {
        throw "Bundled bilingual OCR failed with exit code $($process.ExitCode)"
    }
} catch {
    Write-Host "::error::$($_.Exception.Message)"
    if (Test-Path -LiteralPath $log) {
        Get-Content -LiteralPath $log -Tail 12 | ForEach-Object {
            $message = $_.Replace('%', '%25').Replace("`r", '%0D').Replace("`n", '%0A')
            Write-Host "::error::$message"
        }
    }
    foreach ($stream in @($stdout, $stderr)) {
        if ($stream -and (Test-Path -LiteralPath $stream)) {
            Get-Content -LiteralPath $stream -Tail 10 | ForEach-Object {
                $message = $_.Replace('%', '%25')
                Write-Host "::error::$message"
            }
        }
    }
    throw
} finally {
    $env:PATH = $oldPath
    Remove-Item -LiteralPath $fixtureDir -Recurse -Force
}
