# MaenPDF Windows Code Signing and Signed Releases

MaenPDF 7.5.5 changes the Windows publishing path so official GitHub Releases are not published unsigned.

## What this does

The release workflow rebuilds the exact commit that passed `MaenPDF CI`, deploys the Windows runtime, signs `dist/MaenPDF.exe`, builds the Inno Setup installer from that signed application, signs `MaenPDF-Setup.exe`, verifies both Authenticode signatures, performs a silent install smoke test, verifies the installed executable is signed by the same certificate, generates `SHA256SUMS.txt`, and only then creates the GitHub Release.

The existing updater continues to use the GitHub Releases API and GitHub-provided SHA-256 asset digest. Because 7.5.5 is newer than 7.5.4, existing 7.5.4 installations can discover it through the current version-comparison logic.

## Required one-time setup

A trusted Windows Authenticode code-signing certificate is required. Do not commit a `.pfx`, `.p12`, private key, password, or certificate secret to the repository.

Create these two GitHub Actions repository secrets:

- `WINDOWS_SIGNING_PFX_BASE64` — Base64 of the complete PFX file containing the public-trust code-signing certificate and private key.
- `WINDOWS_SIGNING_PFX_PASSWORD` — password protecting that PFX.

PowerShell can produce the Base64 value locally:

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes("C:\path\to\codesigning.pfx")) | Set-Clipboard
```

Add the secrets under the repository's **Settings -> Secrets and variables -> Actions**. The release workflow intentionally fails before publishing if either secret is missing. This prevents a later release from accidentally reverting to an unsigned Windows installer.

## Certificate requirements

Use a certificate intended for Windows code signing and chained to a root trusted by supported Windows versions. Keep using the same publisher identity/certificate lineage for future releases whenever practical so reputation can accumulate. A self-signed certificate does not provide public Windows publisher trust and is not a substitute for Authenticode distribution signing.

The workflow timestamps signatures with an RFC 3161 timestamp server and SHA-256. The private key exists only as a temporary PFX on the ephemeral GitHub-hosted Windows runner and is deleted in an `always()` cleanup step.

## Release behavior

The current public release is v7.5.4. After these files are uploaded and the signing secrets are configured, a successful `main` CI run for the version 7.5.5 commit triggers the signed release workflow. The release publishes:

- `MaenPDF-Setup.exe` — Authenticode-signed installer containing an Authenticode-signed `MaenPDF.exe`.
- `SHA256SUMS.txt` — SHA-256 checksum for the final signed installer.

The workflow refuses to recreate an existing tag. To publish another build later, bump the semantic version consistently in `CMakeLists.txt`, `app/main.cpp`, `installer/MaenPDF.iss`, and the corresponding release audit expectations.

## Verification on Windows

After downloading an official release, users or maintainers can inspect the signature with:

```powershell
Get-AuthenticodeSignature .\MaenPDF-Setup.exe | Format-List Status,StatusMessage,SignerCertificate,TimeStamperCertificate
```

`Status` should be `Valid`. A valid signature significantly improves publisher provenance and helps reduce false-positive and reputation warnings, but no legitimate publisher can guarantee that every antivirus engine will never flag a future build. If a signed clean build is still detected, submit that exact signed file/hash through the antivirus vendor's official false-positive process rather than changing the program to evade detection.
