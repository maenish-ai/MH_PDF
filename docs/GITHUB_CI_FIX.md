# GitHub CI fix — QtPdf extension

Qt PDF is distributed as a Qt extension in modern Qt releases. The previous workflow used
`jurplel/install-qt-action` with `modules: qtpdf`, which can fail with the upstream error
`The packages ['qtpdf'] were not found while parsing XML of package information`.

The v6.0.1 workflow bypasses that failing integration path and invokes the current aqtinstall
source directly. Qt 6.11.2 is fully pinned and QtPdf is explicitly installed for Windows,
Android arm64-v8a, and Android armeabi-v7a. Each build verifies `Qt6PdfConfig.cmake` before
running CMake so a dependency-install problem is reported clearly before compilation.

If GitHub later ships a fixed stable aqtinstall/install-qt-action combination, this workaround
can be replaced with a pinned stable release after CI validation.
## v6.0.3 code/build correction

Run #3 progressed past Qt installation, QML parsing, Windows generator selection, and Android SDK setup. It exposed two real blockers:

1. Qt 6.11 declares `QPdfDocument::render()` and `getAllText()` as non-const. `QtPdfBackend` now stores `m_doc` as `mutable`, preserving the const read-only backend contract while allowing Qt to update internal caches.
2. Android target Qt requires a host Qt when cross-compiling. Both CI and Play workflows now pass `-DQT_HOST_PATH="$RUNNER_TEMP/Qt/${QT_VERSION}/gcc_64"` and validate the host installation created by `--autodesktop`.

Do not remove these guards unless the Qt backend/API or Android toolchain architecture changes.

- GitHub Actions runtime hygiene: checkout v7.0.1, setup-python v7.0.0, setup-android v4.0.4, and upload-artifact v7.0.1 all use Node.js 24, eliminating the Node.js 20 deprecation annotations.

## v6.0.8 Android artifact-order correction

GitHub Actions run #7 proved that the Android application itself compiles and that Qt produces a correctly signed release APK (`android-build-release-signed.apk`). The subsequent AAB target regenerates Android packaging outputs and can replace the APK output directory with an unsigned intermediate before a later collection step runs. The CI now verifies and copies the signed APK into `dist-android/MaenPDF-Installable-Test.apk` immediately after the APK target, then builds and collects the AAB separately. A release audit enforces this ordering.
