# MaenPDF 7.2

MaenPDF is a free, local-first, bilingual PDF workspace written in C++17 with Qt/QML. Its product goals are **fast, private, free and open**. Documents are processed on the user's device; the core application does not require a cloud account or subscription.

## 7.2 Interaction & Performance Release
- Complete English/Arabic application catalogs with persistent language choice and automatic LTR/RTL layout.
- Continuous multi-page viewer: the mouse wheel/touchpad moves naturally from page to page instead of trapping the user on one page.
- Virtualized/reused page delegates, bounded LRU render cache and low-memory mode for old and new devices.
- Pointer drag selects real source-PDF text and exposes Copy Text / Highlight Selection.
- Text tool inserts text at the clicked page position.
- Highlight tool follows real text geometry, with an area fallback for scanned/image-only pages.
- Draw tool previews ink live while dragging and commits one undoable stroke with selectable brush color/width.
- Crop is previewed first, has adjustable corner handles, and only changes the page after Apply.
- Redaction is previewed first and only commits after Apply; MaenPDF saves edited output as flattened PDF pages.
- Search, thumbnails, zoom, Page Up/Page Down navigation, tabs, Recent Files, drag/drop and recovery snapshots.
- Create/open/save, combine, extract, insert/delete/duplicate/move/copy/paste/rotate/crop pages.
- Image/signature overlays, watermark, page numbers and Bates numbering.
- Native Windows Print, Print Current Page and Print Preview.
- Optional local qpdf tools for AES-256 protection, decrypt, optimize, linearize, repair and split.
- Optional local Tesseract OCR and optional LibreOffice conversion bridge.
- Clean Windows upgrades and a stable Android package identity for update continuity.

## Performance model
Only visible/nearby page delegates are instantiated. Render images are requested at the displayed size and loaded asynchronously, while Qt PDF document access is serialized. Long local operations keep the native event loop alive so Windows remains responsive. The render cache has a fixed memory budget and can be reduced further with Low Memory Mode.

## Clean upgrade contract
The installer uses the same stable AppId for updates and replaces the application directory before copying a new build, preventing obsolete runtime/plugins from surviving beside the new version. The one-time pre-7.1 settings migration remains in place; later compatible settings are preserved unless the user selects the installer reset task. User PDF documents are never deleted by the installer.

## Build contract
- CMake 3.21+
- C++17
- Qt 6.11.x with Qt PDF
- Desktop: Qt Widgets + Qt Print Support
- CI Qt: 6.11.2
- Android: target API 36, minimum API 28, JDK 21, NDK 27.2.12479018

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.2/<kit>
cmake --build build --config Release
python scripts/preflight.py
```

## Honest capability boundary
MaenPDF 7.2 is not yet a full object-tree PDF editor. Existing source PDF text/image objects are not structurally rewritten, certificate-backed digital signatures and AcroForm authoring remain disabled, and true object-tree redaction is not claimed. Text selection is available for source-backed text PDFs; pages rasterized by crop or image-only scans require OCR for selectable text.

## Privacy and community
Optional qpdf, Tesseract and LibreOffice providers are discovered at runtime and are not required by the lightweight core. Donation support is optional and does not unlock functionality.

Code is licensed under GPL-3.0-or-later. Use of the MaenPDF name and logo is governed separately by `BRAND_POLICY.md`.
