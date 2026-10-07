# GitHub and Store Release Guide — MaenPDF 7.4.1

## Repository layout
Upload the **contents** of the MaenPDF folder to the repository root, including `.github/`.

## Continuous integration
Every push to `main` or `develop`, and every pull request, runs:
1. source/reliability and QML wiring audits;
2. bilingual EN/AR localization audit;
3. release/packaging audit;
4. engine-v7 architecture, security, performance, interaction and shortcut audits;
5. Windows 10/11 x64 build, native runtime smoke tests, interaction stress test and installer smoke test;
6. Android multi-ABI installable APK build and signature verification.

The build jobs depend on the audit job, so packaging cannot start when a release gate fails. Standard CI intentionally does **not** upload a Windows Portable artifact and does not build a Play AAB on every push.

## Stable Android identity
- Package ID: `org.orbispdf.app`
- versionName: `7.4.1`
- versionCode: `70401`
- minSdk: 28
- target/compile SDK: 36

After first publication, never change the package ID or production signing identity. Increase versionCode on every production Android upload.

## CI APK versus production APK
Standard CI creates `MaenPDF-Android-CI-Test.apk` with an ephemeral CI-only signing key. It is installable for testing, but the key changes between runs and therefore it is **not** the long-term production update identity.

The manual `.github/workflows/play-release.yml` workflow uses persistent protected GitHub signing secrets and produces:
- `MaenPDF-Android-Release.apk` for direct installation/distribution;
- `MaenPDF-Android-Play.aab` for Google Play.

Required secrets: `ANDROID_KEYSTORE_BASE64`, `ANDROID_KEY_ALIAS`, `ANDROID_KEYSTORE_PASSWORD`, and `ANDROID_KEY_PASSWORD`. Signing material must never be committed to the repository.

## Windows artifact
The normal Windows release artifact is `MaenPDF-Windows-Setup` containing `MaenPDF-Setup.exe`. `dist/` exists only as an internal staging directory used to test the exact deployed runtime before building the installer.

## Compatibility
The current line is configured for Windows 10/11 x64 and Android API 28+. Older operating systems require separate toolchains and should not be advertised as supported until they have their own CI/regression matrix.

## Build authority
Local audits catch source, wiring, localization, packaging and policy regressions. The hosted GitHub Actions Windows/Android run remains the authoritative native compile/package/runtime gate. Never describe a source bundle as GitHub-green until that exact commit has completed successfully.
