# MaenPDF Roadmap

## Implemented baseline — 7.0

- local-first EN/AR reader/editor workspace;
- real document tabs and recent files;
- lazy rendering + low-memory mode;
- password opening;
- page organize operations;
- overlay text/image/highlight/ink/signature image;
- crop, watermark, numbering, Bates;
- secure flattened redaction;
- qpdf local tools;
- optional EN+AR Tesseract OCR workflow;
- compare/export images/image-to-PDF/safe flatten;
- optional LibreOffice bridge;
- recovery/safe save/Windows installer/Android packaging;
- GPL source + separate MaenPDF brand policy.

## Next verified engine work

1. Structural writer provider for direct editing of existing PDF text/image objects.
2. Standards-aware PDF annotations rather than appearance-only overlays.
3. AcroForm filling first, then form authoring.
4. Certificate-backed digital signatures / PAdES with signature validation UI.
5. Object-level true redaction with sanitization of related hidden data.
6. PDF/A validation/conversion where an appropriate open provider can be integrated legally.
7. Parser/renderer isolation in a restricted worker process for stronger hostile-document containment.
8. Continuous fuzzing and a larger interoperability corpus.

Large or optional features should remain feature packs/providers rather than inflating the base application.
