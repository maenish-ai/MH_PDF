# Optional local providers

MaenPDF 7 can discover optional tools from the operating-system PATH or from these application-relative locations:

- `tools/qpdf/qpdf[.exe]`
- `tools/tesseract/tesseract[.exe]` with optional `tessdata/`
- `tools/libreoffice/program/soffice[.exe]`

These providers are deliberately not required by the base build. This keeps the core application small. A distributor may package verified provider binaries separately as local feature packs, subject to each provider's license and redistribution terms.

MaenPDF never downloads or installs these tools silently.
