# MaenPDF 7.1.0 — Build Status

## Local verification
All source-level release gates in this bundle pass:
- source/reliability audit;
- EN/AR localization parity audit;
- release/packaging audit;
- engine v7 architecture audit;
- security audit;
- performance audit;
- open-source/brand audit;
- JSON, XML and GitHub Actions YAML parsing.

Run locally before every push:

```bash
python scripts/preflight.py
```

## Native build authority
This working environment does not contain the full Qt 6.11 desktop/Android SDK, so the GitHub Actions workflow remains the authoritative native compilation, packaging and runtime smoke-test gate.

## Proven baseline
GitHub Run #11 was fully green for MaenPDF 6.0.11. Run #12 for MaenPDF 7.0.0 proved Source Audit and Android green, and exposed a Windows QML startup property error. The 7.0.1 stabilization removed that error and added a real PDF-open smoke test so lazy QML delegates are instantiated during CI.

## 7.1.0 Professional Foundation changes
- Windows installer now performs a clean application-directory replacement on every update, preventing obsolete DLLs/plugins from surviving an upgrade.
- A one-time settings-generation migration clears incompatible pre-7.1 application state without touching user PDF documents.
- Future 7.1+ installs preserve compatible settings unless the user chooses the installer reset task.
- Qt desktop builds now link Qt Widgets and Qt Print Support and expose Print, Print Current Page and Print Preview.
- `windeployqt` packaging explicitly validates `Qt6Widgets.dll` and `Qt6PrintSupport.dll` in addition to the existing Qt PDF and VC143 runtime checks.
- CI installer smoke testing now simulates stale legacy settings and an obsolete application file and verifies that both are removed by the clean-upgrade path.
- The desktop menu surface now includes File, Edit, View, Document, Pages, Comment, Forms, Protect, Convert, Tools, Window, Language and Help.
- Ctrl+K command palette is searchable.
- Settings use an explicit schema version.

## Android
The standard CI workflow continues to produce `MaenPDF-Installable-Test.apk` with an ephemeral CI-only signing key and verifies it with `apksigner`. The AAB remains a validation artifact; production Play signing stays isolated in `.github/workflows/play-release.yml` with persistent secrets.
