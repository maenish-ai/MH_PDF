# Orbis PDF Professional — Quality Gate

Orbis should not be marketed as an Acrobat-class replacement until the complete public-release matrix passes. Engineering milestones may be distributed for testing with their capability boundary stated clearly.

## Release-blocking gates
- **No data loss:** save uses write → reopen/verify → replacement with rollback for local files.
- **Interoperability:** exported PDFs must be opened by multiple independent readers before public release.
- **Security:** encrypted desktop output uses standard PDF encryption through the configured provider; passwords are never stored by Orbis.
- **Undo/redo:** destructive page/edit operations require history coverage or explicit documentation.
- **Memory:** opening large PDFs is lazy; render cache is bounded.
- **Stress:** test 1, 10, 100, 500 and 1,000-page documents, malformed files, mixed page sizes and very large images.
- **Localization:** English interface catalog contains no Arabic characters; Arabic UI is RTL and contains no untranslated English UI words (technical file-extension patterns excluded).
- **Recovery:** modified sessions receive periodic verified recovery snapshots.
- **Android:** content URI workflows, lifecycle/recovery, signing and Play policy review must pass before production submission.
- **CI:** source/localization/release/engine audits must pass before platform builds.

## Current v6 limitation
Imported pages remain source-backed and efficient to view, but edited export is appearance-based rather than lossless PDF object-tree editing. OCR, digital signatures, AcroForms and true redaction are deliberately disabled capabilities.
