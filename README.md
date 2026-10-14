# MH Smart PDF Importer — consolidated development package

Target: maenish-ai/MH_PDF, Qt 6.11.

Run `python apply_to_repo.py /path/to/MH_PDF` against an extracted clean repository checkout. Inspect git diff, commit on a branch, then build with GitHub Actions. Open via Ctrl+Shift+I on desktop.

Implemented: direct public PDF download, HTML link discovery, redirect handling (max 8), progress, cancellation, streamed temporary-file writes, atomic save, duplicate-safe filenames and automatic opening in MH_PDF.

**Not a verified final release.** Qt compiler/Android builds were not available in this environment. Does not support JavaScript-rendered viewers, authenticated sessions, DRM, or segmented PDF reconstruction. Android storage integration needs validation. DNS-resolved private IP blocking is incomplete, so do not use untrusted URLs on sensitive networks. Limits: PDF 300 MiB; HTML 4 MiB. Build/test before release.
