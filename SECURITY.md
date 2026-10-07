# MaenPDF Security Policy

## Security model

MaenPDF processes documents locally. The application does not need a MaenPDF cloud account and does not upload user PDFs to a MaenPDF server.

Release signing credentials, Android keystores, store credentials, and donation/payment secrets must never be committed to this repository. Official release secrets belong only in protected release infrastructure.

## Binary and runtime hardening

Release builds enable platform mitigations where the toolchain supports them: control-flow protection and DEP/ASLR-compatible flags on MSVC, stack protection and RELRO/NOW on Unix-like targets, and a bounded Qt image-decoder allocation limit selected from the adaptive performance profile. These measures reduce exploitability but do not replace secure parsing or code review.

MaenPDF is open source, so its security model never depends on hiding source code or embedding secrets in the executable. Official-update trust comes from protected signing keys, reproducible CI inputs, release checks and the separate MaenPDF brand/signing policy.

## Untrusted PDFs

PDF files are treated as untrusted input. MaenPDF keeps rendering caches bounded, uses local temporary directories for transformation workflows, applies process timeouts to optional command-line providers, and does not execute PDF JavaScript.

## Optional local providers

qpdf, Tesseract, and LibreOffice integrations are optional local tools. MaenPDF invokes them with argument arrays rather than through a command shell. Users control when these operations run.

## Reporting a vulnerability

Do not publish an exploitable vulnerability with working malicious files before maintainers have had a reasonable opportunity to investigate. Use a private security-reporting channel on the official repository when available.
