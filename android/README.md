# Android release contract

- Package ID: `org.orbispdf.app`. **Do not change it after the first Play release.**
- Version: `6.0.10`, versionCode `60010`.
- Modern runtime target: Android 9 / API 28 and newer.
- Google Play compile/target API: 36.
- CI toolchain: Qt 6.11.2, JDK 21, Android NDK 27.2.12479018.
- GitHub CI builds an unsigned APK and AAB for validation.
- ARM64 and ARMv7 Qt SDKs are installed in CI so the modern package can cover both 64-bit and 32-bit ARM devices supported by this Qt line.

## Updates without data loss
Keep the same package ID and the same signing key and only increase `QT_ANDROID_VERSION_CODE`. Android then treats the next release as an update of the same app. Preferences, recovery data and app-private files remain in the app data area; user PDF files remain user documents.

## Signing
Never commit a `.jks`/`.keystore` file or passwords. Configure Play App Signing and store upload-key material in GitHub Actions secrets when the Play Console project is created.

## Legacy Android
Android versions older than API 28 are outside the supported runtime range of the modern Qt line used by MaenPDF v6. They require a separately maintained legacy branch and must not be advertised as supported until that branch is built and tested.

A manual GitHub workflow, `.github/workflows/play-release.yml`, is included for signed Play artifacts once the four signing secrets documented in `docs/GITHUB_RELEASE.md` are configured. It does not run on ordinary pushes.

## CI test APK installation

The normal CI workflow creates `MaenPDF-Installable-Test.apk`, a release-mode APK signed with an ephemeral CI-only test key and verified with `apksigner`. It is intended only for direct device testing. The accompanying `.aab` is not directly installable on Android; it is retained only for bundle validation. For Play distribution, use the separate `play-release.yml` workflow with the real persistent upload keystore stored in GitHub Secrets.

Because the CI test key is generated afresh on every workflow run, uninstall a test build from the device before installing an APK produced by a different CI run if Android reports a signature/update conflict. Never use the CI test key for production or Google Play releases.
