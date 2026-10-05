# GitHub and Store Release Guide — MaenPDF 6

## Repository layout
Upload the **contents** of the MaenPDF folder to the repository root, including `.github/`.

## Continuous integration
Every push to `main` or `develop`, and every pull request, runs:
1. source/reliability audit;
2. bilingual localization audit;
3. release/packaging audit;
4. engine-v6 architecture audit;
5. Windows 10/11 x64 build and deployment;
6. Android APK and AAB build.

The build jobs depend on the audit job, so packaging cannot start when a release gate fails.

## Stable Android identity
- Package ID: `org.orbispdf.app`
- v6 versionName: `6.0.8`
- v6 versionCode: `60008`
- minSdk: 28
- target/compile SDK: 36

After first publication, never change the package ID or Play signing identity. Increase versionCode on every Play upload.

## Signing
CI intentionally produces unsigned validation artifacts. Do not commit keystores or passwords. When a Play Console application is created, enable Play App Signing and keep the upload key outside the repository (for example in GitHub Actions secrets).

## Compatibility
The modern v6 line is tested/configured for Windows 10/11 and Android API 28+. Windows 7/8 and Android below API 28 require separately maintained legacy toolchains and should not be advertised as supported until their own CI/regression matrix exists.

## First GitHub run
A local audit cannot prove the behavior of a hosted runner. After the first push, open **Actions → MaenPDF CI** and confirm all jobs are green. If a runner/provider changes upstream, use the failing GitHub log as the source of truth rather than weakening the audits.

## Play signing workflow
`.github/workflows/play-release.yml` is manual-only. Before using it, create these GitHub Actions secrets: `ANDROID_KEYSTORE_BASE64`, `ANDROID_KEY_ALIAS`, `ANDROID_KEYSTORE_PASSWORD`, and `ANDROID_KEY_PASSWORD`. The workflow decodes the upload keystore only inside the hosted runner and uses Qt's `QT_ANDROID_SIGN_AAB`/`QT_ANDROID_SIGN_APK` signing path.
