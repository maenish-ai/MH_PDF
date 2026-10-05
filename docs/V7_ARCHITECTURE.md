# MaenPDF 7 Architecture

MaenPDF 7 is a local-first modular PDF workspace.

## Core layers

1. **Document session layer** — `DocumentManager` owns independent `PdfDocument` sessions and provides real multi-document tabs.
2. **Lazy rendering layer** — `PageModel`, `QtPdfBackend`, and `SessionImageProvider` render pages on demand with a bounded LRU cache.
3. **Editing/export layer** — page operations and visual edits are applied locally. Current export intentionally flattens visual edits so redaction areas are not left as recoverable hidden source content in the exported copy.
4. **Optional local tools layer** — `PdfToolsService` discovers qpdf, Tesseract, and LibreOffice at runtime. Missing optional providers do not prevent MaenPDF from starting.
5. **Settings/localization layer** — `AppSettings` stores recent files, theme and low-memory preferences; `LanguageManager` switches the complete application between English/LTR and Arabic/RTL.

## No-cloud rule

The MaenPDF core does not contain a document-upload client. External links are opened only by explicit user action. Optional PDF/OCR/office operations invoke local executables only.

## Capability honesty

Structural editing of existing PDF text objects, certificate-backed digital signatures, AcroForm authoring, and object-level redaction remain disabled until a verified structural writer provider and a regression corpus are integrated. MaenPDF 7 does provide overlay editing, secure flattened redaction, password opening, AES-256 protected copies when qpdf is available, OCR when Tesseract and qpdf are available, comparison, split, repair, optimization, image export, and safe flattening.
