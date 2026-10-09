# MaenPDF 7.5.0

MaenPDF is a free, local-first, bilingual PDF workspace written in C++17 with Qt/QML. Its product goals are **fast, private, free and open**. Documents are processed on the user's device; the core application does not require a cloud account, subscription, telemetry service or mandatory network connection.

## 7.5.0 Highlight, Unicode & Full Screen Release
- Fixes the Qt 6.11 cross-platform compile regression in inserted-text move/resize and adds a compiler API regression audit to CI.
- Full English/Arabic catalogs with persistent language choice and automatic LTR/RTL layout.
- Inserted text is a structured MaenPDF overlay: selectable, draggable, resizable, editable, deletable, partially strikeable and fully Undo/Redo aware.
- Fixed automatic viewport/sidebar jumping by decoupling current-page tracking from ListView currentIndex and limiting explicit repositioning to navigation actions.
- Adaptive performance detects available system RAM and selects Eco/Balanced/Performance budgets automatically; 4–6 GB computers receive tighter render/cache budgets while edit history remains unlimited.
- Release builds add compiler/linker hardening and image-decoder allocation limits while keeping all document processing local.
- Continuous virtualized multi-page reader with tabs, thumbnails, search, text selection, mouse-wheel navigation and debounced zoom rendering that prevents rapid zoom commands from creating render storms.
- Pointer/text selection, text insertion, multi-color geometry-aware highlight, a freehand Highlight Pen, live ink, crop preview and staged flattened redaction.
- Incremental/decimated live ink: long strokes no longer clone and repaint the complete path on every pointer event.
- Unlimited Undo/Redo history with lightweight patch/vector commands, smaller overlay budgets, bounded LRU rendering, adaptive prefetch and quantized render sizes for old hardware. Highlight/redaction/draw undo keeps touched patches instead of unnecessary full-page copies.
- Recovery uses an idle-debounced local journal instead of exporting the full PDF every minute while the user is active.
- Optional heavy local tools run through background jobs so the QML interface stays responsive.
- Accelerated Windows graphics by default, with Safe Graphics mode and automatic startup-failure fallback for problematic GPU drivers.
- Native Windows Print, Print Current Page and Print Preview. PDF Full Screen Mode shows the document alone in true OS fullscreen; Esc or Ctrl+L exits.
- Page insert/delete/duplicate/move/copy/paste/rotate/crop/extract/combine/split, watermark, page numbers and Bates numbering.
- Optional local qpdf tools for AES-256 protection, decrypt, optimize, linearize, repair/check and batch optimization.
- Optional local Tesseract OCR (English + Arabic default) and optional LibreOffice conversion bridge.
- Compare PDFs, image/page export, image-to-PDF and privacy-oriented safe flattening. Local form fields (text, checkbox, radio and dropdown) are usable and flatten into the saved PDF for broad compatibility.
- Acrobat-familiar shortcuts (including Ctrl+= / Ctrl+- zoom, Ctrl+K Preferences, V/H/T/U/D/C tools), a separate Ctrl+Shift+P Command Palette, Recent Files, drag/drop, unsaved-change guards and clean Windows upgrades.

## Performance model
Only visible/nearby page delegates are instantiated. Page images load asynchronously, Qt PDF document access is mutex-serialized and the shared render cache is bounded and serialized. Nearby zoom sizes share cache buckets rather than creating a bitmap for every pixel-size variation. Low Memory Mode reduces cache/render budgets further while keeping edit history available.

Live drawing is intentionally split into two phases: a lightweight incremental Canvas preview while the pointer is moving, followed by one committed undoable overlay operation when the stroke ends. Drawing undo stores only the touched pixel patch instead of retaining another full-page overlay bitmap for every stroke. Heavy optional tools (OCR, compare, optimize, repair, split, conversion, etc.) have background entry points and show a non-blocking processing state.

## Recovery model
MaenPDF waits for user idle time before writing recovery state. The recovery journal stores page structure and only the local overlay/base images needed to reconstruct edits. It does **not** rasterize and export the whole document on a timer. Recovery data remains local in the application data directory and is removed after a successful save/discard.

## Clean upgrade contract
The installer uses the same stable AppId for updates and replaces the application directory before copying a new build, preventing obsolete runtimes/plugins from surviving beside the new version. User PDF documents are never deleted by the installer.

## Build contract
- CMake 3.21+
- C++17
- Qt 6.11.x with Qt PDF and Qt Concurrent
- Desktop: Qt Widgets + Qt Print Support
- CI Qt: 6.11.2
- Android: target API 36, minimum API 28, JDK 21, NDK 27.2.12479018

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.2/<kit>
cmake --build build --config Release
python scripts/preflight.py
```

## Release artifacts
Normal CI validates Windows and Android and publishes **MaenPDF-Windows-Setup** plus an ephemeral-key **MaenPDF-Android-CI-Test** APK. It intentionally does **not** publish a Windows Portable package.

The manual `MaenPDF Android Release` workflow uses persistent GitHub signing secrets and produces `MaenPDF-Android-Release.apk` plus `MaenPDF-Android-Play.aab`. See `docs/ANDROID_RELEASE_SIGNING.md`.

## Honest capability boundary
MaenPDF 7.4 is not yet a full object-tree PDF editor. Existing source PDF text/image objects are not structurally rewritten, certificate-backed digital signatures and **native AcroForm authoring** are not claimed, and object-tree redaction is not claimed. MaenPDF local form overlays are functional and flatten on save, but they are deliberately not advertised as native AcroForm fields until a structural form backend is integrated and verified.

Code is licensed under GPL-3.0-or-later. Use of the MaenPDF name and logo is governed separately by `BRAND_POLICY.md`.
