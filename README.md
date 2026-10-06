# MaenPDF 7.1

MaenPDF is a free, local-first, bilingual PDF workspace written in C++17 with Qt/QML. The project is designed around four goals: **fast, private, free and open**. Core document work happens on the user's device; MaenPDF does not require a cloud account or subscription.

## 7.1 Professional Foundation
- Complete English/Arabic application catalogs with persistent language choice and automatic LTR/RTL layout.
- Professional desktop menu surface: File, Edit, View, Document, Pages, Comment, Forms, Protect, Convert, Tools, Window, Language and Help.
- Searchable `Ctrl+K` command palette.
- Native desktop Print, Print Current Page and Print Preview through Qt Print Support and the operating-system printer dialog.
- Clean Windows upgrades: the application directory is replaced on every update so obsolete Qt plugins/runtime DLLs cannot remain mixed with the new build.
- One-time migration from pre-7.1 user state plus an explicit settings schema for future compatible upgrades.
- Tabs, Recent Files, drag/drop, document properties, preferences, dark mode and low-memory mode.
- Lazy, source-backed PDF pages and bounded LRU render cache so large PDFs are not rasterized into RAM up front.
- Search in source PDF text, thumbnails, zoom, page navigation and recovery snapshots.
- Create/open/save, combine, extract, insert/delete/duplicate/move/copy/paste/rotate/crop pages.
- Overlay text, image, highlight, ink, signature image, watermark, page numbers and Bates numbering.
- Safe flattened redaction, safe flattened export and visual PDF comparison.
- Optional local qpdf tools for AES-256 protection, decrypt, optimize, linearize, repair and split.
- Optional local Tesseract OCR (`eng+ara` by default) and optional LibreOffice conversion bridge.
- Android `content://` import/export and signed CI test APK workflow.

## Printing
Windows builds include system printing. The File menu contains Print, Print Current Page and Print Preview. The native print dialog supplies installed printers, copies, ranges and printer-specific capabilities. Printing uses the same local page renderer as the viewer and does not upload the document.

Android keeps the Print commands visible but disabled until a native Android Print Framework provider is integrated; this avoids pretending an unsupported path is complete.

## Clean upgrade contract
The installer uses the same stable AppId for updates. Before copying a new build it cleans the MaenPDF application directory, preventing files from an older runtime from surviving the update. The first 7.1+ installation also resets incompatible pre-7.1 application preferences/cache. Future 7.1+ updates preserve compatible preferences unless the user selects **Reset MaenPDF preferences and cache** during setup. User PDF documents are never stored in the application directory and are never deleted by this process.

## Build contract
- CMake 3.21+
- C++17
- Qt 6.11.x with Qt PDF
- Desktop: Qt Widgets + Qt Print Support for native printing
- CI Qt: 6.11.2
- Android: target API 36, minimum API 28, JDK 21, NDK 27.2.12479018

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.2/<kit>
cmake --build build --config Release
```

Run all local release gates before pushing:

```bash
python scripts/preflight.py
```

## Honest capability boundary
MaenPDF 7.1 is not yet a full object-tree PDF editor. Existing PDF text/image objects are not rewritten structurally, certificate-backed digital signatures and AcroForm authoring remain disabled, and those menu entries are deliberately unavailable rather than presented as completed features. The architecture keeps these capabilities behind explicit engine/provider boundaries for future development.

## Privacy and community
Documents are processed locally. Optional qpdf, Tesseract and LibreOffice providers are discovered at runtime and are not bundled into the lightweight core. Donation support is optional and does not unlock core functionality.

Code is licensed under GPL-3.0-or-later. Use of the MaenPDF name and logo is governed separately by `BRAND_POLICY.md`.
