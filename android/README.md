# MaenPDF Android

MaenPDF 7.4.0 targets Android API 36 with minimum API 28. The internal package ID remains `org.orbispdf.app` for update continuity with earlier builds. This source bundle uses versionName `7.4.0` and versionCode `70400`.

## CI test installation

The standard CI workflow creates `MaenPDF-Android-CI-Test.apk`, signs it with an ephemeral CI-only key and verifies it with `apksigner`. It can be installed directly for testing on a compatible Android device. Because the CI test key changes between workflow runs, uninstall an older CI-test build first if Android reports a signing-key mismatch.

An `.aab` is a Play/Bundle artifact and is not installed directly on a phone.

## Production updates

For a production Play release:

- keep the same package ID;
- keep the same signing key (the production signing key);
- increase `QT_ANDROID_VERSION_CODE` for every update;
- keep release signing secrets outside the repository.

## Local file behavior

MaenPDF supports Android document-provider `content://` URIs. Documents are processed locally; there is no MaenPDF document cloud service.
