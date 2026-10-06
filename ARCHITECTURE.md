# MaenPDF 6 — Architecture

## Product rule
A feature is presented as production-ready only when the active engine reports that capability and the release suite covers its principal failure modes. Unsupported Acrobat-class features stay disabled rather than being represented by placeholder controls.

## Layers
1. `ui/`: original QML workspace, language-neutral through translation keys.
2. `core/LanguageManager`: EN/AR catalog, persistence and LTR/RTL direction.
3. `core/PdfDocument`: document controller, history, safe save, recovery, Android URI bridging and export.
4. `core/PageModel` + `PageImageProvider`: source-backed pages, lazy rendering and bounded LRU image cache.
5. `IPdfBackend`: replaceable PDF-reader contract. v6 ships `QtPdfBackend`.
6. `EngineCapabilities`: runtime truth about supported/unsupported features.
7. Security provider: optional `qpdf` desktop integration for standard encrypted export. Passwords are not persisted.
8. Diagnostics: `AppLogger` writes application diagnostics without security secrets.

## v6 memory model
Opening a PDF creates lightweight page records that reference a backend/source page. It does **not** render all pages during open. Rendering occurs when the image provider requests a page. Render results are held in a bounded LRU cache. Pages that receive overlay edits keep only their overlay raster plus source reference.

## Structural-engine target
A later writer backend must preserve and edit PDF object/content streams for true text/image/vector editing, standard annotations, AcroForms, certificate signatures, true redaction and incremental updates. OCR remains a separate provider because recognition and PDF parsing are distinct responsibilities.

## Non-negotiable public-release tests
Generated/scanned/encrypted/malformed/RTL/mixed-size/large-document corpus, cross-reader interoperability, save/rollback, crash recovery, low-disk-space behavior, Android lifecycle and document picker, accessibility, parser fuzzing, memory budgets and signed-store package validation.

## 7.2 interaction layer
The document workspace is a virtualized vertical `ListView`. Each visible page delegate requests a page render at its displayed size and owns only transient interaction state. Persistent edits are committed to `PdfDocument` and participate in the undo stack.

Source-backed PDF text selection is provided by `IPdfBackend::textSelection()` and the Qt PDF implementation delegates to `QPdfDocument::getSelection()`. Selection bounds are normalized and mapped back through page rotations before QML renders them. Crop and redaction are staged in QML and are committed only after explicit Apply. Ink is previewed in a QML Canvas and committed as one styled stroke.

Page images load asynchronously. `QtPdfBackend` serializes access to its `QPdfDocument`; render calls use page snapshots and closed tabs are briefly retired before deletion to protect in-flight image-provider requests.
