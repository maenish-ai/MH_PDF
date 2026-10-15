param(
    [Parameter(Mandatory = $true)][string]$File,
    [Parameter(Mandatory = $true)][string]$PfxPath,
    [Parameter(Mandatory = $true)][string]$PfxPassword,
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
    throw "File to sign was not found: $File"
}
if (-not (Test-Path -LiteralPath $PfxPath -PathType Leaf)) {
    throw "PFX file was not found: $PfxPath"
}
if ([string]::IsNullOrWhiteSpace($PfxPassword)) {
    throw "PFX password is empty"
}

$kitsBin = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
$signtool = $null
if (Test-Path $kitsBin) {
    $signtool = Get-ChildItem -Path $kitsBin -Recurse -Filter signtool.exe -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\x64\\signtool\.exe$' } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
}
if (-not $signtool) {
    $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($cmd) { $signtool = Get-Item $cmd.Source }
}
if (-not $signtool) {
    throw "signtool.exe was not found on this Windows runner"
}

Write-Host "Signing $File with SHA-256 and RFC3161 timestamp..."
& $signtool.FullName sign `
    /f $PfxPath `
    /p $PfxPassword `
    /fd SHA256 `
    /tr $TimestampUrl `
    /td SHA256 `
    /d "MaenPDF" `
    $File
if ($LASTEXITCODE -ne 0) { throw "signtool sign failed with exit code $LASTEXITCODE" }

& $signtool.FullName verify /pa /all /v $File
if ($LASTEXITCODE -ne 0) { throw "Authenticode verification failed for $File" }

$signature = Get-AuthenticodeSignature -FilePath $File
if ($signature.Status -ne 'Valid') {
    throw "PowerShell Authenticode verification is not valid: $($signature.Status)"
}
if (-not $signature.SignerCertificate) {
    throw "No signer certificate was found after signing"
}

Write-Host "Authenticode valid. Publisher certificate subject: $($signature.SignerCertificate.Subject)"
Write-Host "Certificate thumbprint: $($signature.SignerCertificate.Thumbprint)"
