# MaenPDF Security Policy

## Security model

MaenPDF processes documents locally. The application does not need a MaenPDF cloud account and does not upload user PDFs to a MaenPDF server.

Release signing credentials, Android keystores, store credentials, and donation/payment secrets must never be committed to this repository. Official release secrets belong only in protected release infrastructure.

## Untrusted PDFs

PDF files are treated as untrusted input. MaenPDF keeps rendering caches bounded, uses local temporary directories for transformation workflows, applies process timeouts to optional command-line providers, and does not execute PDF JavaScript.

## Optional local providers

qpdf, Tesseract, and LibreOffice integrations are optional local tools. MaenPDF invokes them with argument arrays rather than through a command shell. Users control when these operations run.

## Reporting a vulnerability

Do not publish an exploitable vulnerability with working malicious files before maintainers have had a reasonable opportunity to investigate. Use a private security-reporting channel on the official repository when available.
