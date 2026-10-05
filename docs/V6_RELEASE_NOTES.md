# MaenPDF 6.0 — Engineering Release

- Added complete EN/AR catalog architecture and RTL switching.
- Added localization release gate to prevent mixed-language interface regressions.
- Replaced eager page rasterization with lazy source-backed rendering.
- Added bounded LRU render cache and constrained-device memory policy.
- Added Android content-URI import/export handling.
- Corrected Qt Android manifest integration and PDF VIEW intent.
- Pinned GitHub CI toolchain and added APK/AAB validation builds.
- Retained safe-save verification, rollback, autosave recovery and diagnostics.
- Kept unsupported structural/OCR/signature/redaction capabilities explicitly disabled.
