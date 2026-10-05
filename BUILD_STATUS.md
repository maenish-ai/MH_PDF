# MaenPDF 6.0 — Build Status

Local release gates in this source bundle pass:
- source/reliability audit;
- EN/AR localization audit;
- packaging/release audit;
- engine-v6 architecture audit;
- JSON/XML/YAML syntax parsing.

The current execution environment does not contain the Qt 6.11 SDK, so a native Windows/Android compilation cannot be truthfully certified here. GitHub Actions is therefore the authoritative compile/package gate after upload. The workflows pin the required toolchain and will stop before packaging if an audit fails.

Run locally before every push:

```bash
python scripts/preflight.py
```
## GitHub Actions run #3 diagnosis (2026-10-04)

The third hosted build confirmed that the Windows generator/toolchain is now correct (MSVC 2022) and QtPdf installs successfully. Two later blockers were identified and fixed in this bundle:

- Windows: `QPdfDocument::render()` and `getAllText()` are non-const in Qt 6.11. The backend now keeps the document `mutable` so logically read-only backend operations can use Qt's internal caches without discarding the const backend API.
- Android: Qt cross-compilation now passes `QT_HOST_PATH` to the host desktop Qt installed by `aqt --autodesktop`, and validates that host Qt before CMake configure.

These fixes are also guarded by the source/release audits. The next GitHub Actions run remains the authoritative native compile/package check.


## GitHub Actions Run #4 follow-up

- Windows 10/11 x64: configure, compile, Qt deployment, and artifact upload all passed on GitHub-hosted Windows 2022.
- Android reached Qt/CMake configuration successfully with the host Qt path resolved.
- The remaining Android configure failure was CMake rejecting the desktop-only install rule because the Android executable target is a module library.
- v6.0.4 changes the install rule to provide an explicit Android `LIBRARY DESTINATION`, while retaining normal desktop runtime/bundle destinations.

- GitHub Actions runtime hygiene: checkout v7.0.1, setup-python v7.0.0, setup-android v4.0.4, and upload-artifact v7.0.1 all use Node.js 24, eliminating the Node.js 20 deprecation annotations.

### Android direct-install artifact
The standard CI workflow now generates a release-mode `MaenPDF-Installable-Test.apk` signed with an ephemeral CI-only key, verifies it with Android `apksigner`, and uploads it in the `MaenPDF-Android-Installable-Test` artifact. This fixes the previous situation where CI exposed only an unsigned release APK that Android refused to install. The AAB remains non-installable directly and is for bundle validation only; production Play signing remains isolated in `play-release.yml` with persistent secrets.
