# MaenPDF 6.0

MaenPDF is an original, cross-platform PDF workspace written in C++17 with Qt/QML. Its identity, logo, colors and control layout are intentionally distinct from Adobe Acrobat while targeting the same broad class of PDF workflows.

## v6 highlights
- English and Arabic UI catalogs with persistent language choice.
- English mode contains English interface text only; Arabic mode switches the application to RTL and uses an Arabic interface catalog.
- Lazy, source-backed PDF pages: opening a large document no longer rasterizes every page up front.
- Bounded LRU render cache and lower Android render limits for constrained devices.
- Search in source PDF text, page thumbnails, zoom and navigation.
- Create/open/save standard PDF, combine documents, extract pages, page insert/delete/duplicate/move/copy/paste/rotate.
- Overlay editing: text, image, highlight and ink; watermark and page numbering.
- Undo/redo for core document operations.
- Safe verified save with rollback and periodic crash-recovery snapshots.
- Android `content://` import/export support for document-picker workflows.
- Optional desktop AES-256 protected export through `qpdf` when installed.
- GitHub Actions quality gates followed by Windows 10/11 x64 and Android APK/AAB builds.

## Build contract
- CMake 3.21+
- C++17
- Qt 6.11.x with Qt PDF
- CI pins Qt 6.11.2.
- Android CI pins API 36, min API 28, JDK 21 and NDK 27.2.12479018.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.2/<kit>
cmake --build build --config Release
```

Run the local release gates before pushing:

```bash
python tests/source_audit.py
python tests/localization_audit.py
python tests/release_audit.py
python tests/engine_v6_audit.py
```

## GitHub
Push the contents of this folder to the repository root. `.github/workflows/ci.yml` runs audits first, then Windows and Android builds. Build artifacts are uploaded by GitHub Actions.

## Android identity and updates
The package ID is `org.orbispdf.app`. It is retained internally for Android update continuity even though the visible product name is MaenPDF. After the first Play release, keep the package ID and signing key unchanged and increase `QT_ANDROID_VERSION_CODE` for every update. This allows Android to update the installed application without replacing it as a different app; application-private data remains under the same identity.

## Honest capability boundary
v6 is not yet an Acrobat-equivalent structural PDF editor. Imported PDF pages remain source-backed for efficient viewing, while edits are rendered as overlays on export. True object-tree text/image editing, OCR, certificate-backed digital signatures, AcroForm authoring and true redaction remain disabled in the capability map until a suitable structural writer/provider and a regression corpus are integrated. No placeholder button is presented as if those features were complete.

## Community direction
The intended public release can remain free for community use. Donation links should be optional and should only be added after official, verified donation destinations are selected. Donations must not unlock core PDF functionality.

### Android test install
GitHub CI publishes `MaenPDF-Installable-Test.apk` inside the `MaenPDF-Android-Installable-Test` artifact. That APK is signed with a CI-only test key and verified with `apksigner`, so it can be installed directly on a compatible Android device. Do not try to install the `.aab` directly; use the Play release workflow for production signing and distribution.


## Windows installer

The Windows CI job now produces `MaenPDF-Windows-Setup`, containing a single `MaenPDF-Setup.exe`. The workflow first deploys the full Qt runtime with `windeployqt`, verifies the required Windows platform and Qt PDF DLLs, launches the portable executable through the native Windows platform, builds the Inno Setup installer, installs it silently into a clean test directory, validates the generated desktop shortcut, and launches the application from that shortcut. MaenPDF defaults to Qt Quick software rendering on Windows to avoid silent startup failures caused by incompatible GPU drivers. The setup creates Start Menu and optional Desktop shortcuts named **MaenPDF** and registers MaenPDF as an available PDF opener.

For normal use, download the **MaenPDF-Windows-Setup** artifact and run `MaenPDF-Setup.exe`; do not copy only the portable EXE away from its Qt runtime folder.
