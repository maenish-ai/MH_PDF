# Android production signing

The normal CI APK is intentionally signed with an ephemeral test key. **Do not publish that APK as the long-term production build.** Android updates require the same package ID and the same signing key for every release.

MaenPDF keeps the package ID `org.orbispdf.app`. The manual **MaenPDF Android Release** workflow expects these GitHub Actions secrets:

- `ANDROID_KEYSTORE_BASE64`
- `ANDROID_KEY_ALIAS`
- `ANDROID_KEYSTORE_PASSWORD`
- `ANDROID_KEY_PASSWORD`

Create and protect one long-lived upload/release keystore outside the repository. Never commit `.jks`, passwords or their base64 form. Store an offline backup. Then encode the keystore as base64 and save the value in `ANDROID_KEYSTORE_BASE64`; store the alias/passwords in the other secrets.

When those four secrets are configured, run `.github/workflows/play-release.yml` manually. It builds and verifies the APK first, preserves it as `MaenPDF-Android-Release.apk`, then builds `MaenPDF-Android-Play.aab`. Both are uploaded in the `MaenPDF-Android-Release` artifact.

The workflow intentionally keeps signing material only in the GitHub runner temporary directory.
