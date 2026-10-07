# MaenPDF 7.3 — feature and user-pain matrix (October 2026)

MaenPDF remains **local-first, no account, no mandatory cloud and free/open source**.
This matrix records ideas worth adopting without copying proprietary code or UI.

## What leading PDF tools do well

- Adobe Acrobat / Foxit: structural editing, OCR, forms, security, redaction, signatures, compare and broad compatibility.
- PDF-XChange: deep page/document tools, Bates numbering, compare, sanitize/optimization, form authoring and dense power-user controls.
- SumatraPDF: extremely fast startup, simple tabs/navigation and a searchable Ctrl+K command palette.
- Okular: annotations, bookmarks, forms and signing with a desktop-first workflow.
- Stirling PDF: many local PDF transformations and repeatable workflows.

## Repeated user complaints MaenPDF should avoid

1. Slow launch, UI freezes and excessive background processes.
2. Large memory/disk footprint for basic reading/editing.
3. Subscription/account requirements for basic operations.
4. Forced cloud upload for private documents.
5. AI panels/prompts that obstruct ordinary PDF work.
6. Hidden page-management/redaction tools and over-complicated menus.
7. Cheap/lightweight editors breaking formatting on complex PDFs.
8. Weak behavior on very large PDFs.

## MaenPDF response

### Shipping now
- Continuous virtualized reader, tabs, thumbnails, search and text selection.
- Highlight, ink, text/image overlays, crop, flattened redaction, watermark, page numbers and Bates numbering.
- Page insert/delete/duplicate/reorder/rotate/extract/merge/split.
- Native Windows printing and print preview.
- Local optional qpdf tools: optimize, linearize, repair, check, decrypt/protect.
- Local optional Tesseract OCR (English + Arabic default).
- Local optional LibreOffice bridge.
- Compare, image export, image-to-PDF, safe flatten/privacy sanitize and batch optimize.
- Acrobat-familiar shortcuts, Ctrl+Shift+P command palette, EN/AR and RTL/LTR.
- 7.3.1 performance work: incremental ink preview, background local tools, idle recovery, bounded caches/undo, accelerated graphics by default with safe fallback.

### Must be implemented only with a real backend
- Structural edit/reflow of existing PDF text and images.
- Full AcroForm creation/editing.
- Certificate/PAdES digital signatures and validation.
- Object-level redaction that preserves non-redacted vector content.
- PDF/A, PDF/X and PDF/UA preflight/remediation.

Disabled UI items must stay disabled until these are genuinely implemented and regression-tested.
