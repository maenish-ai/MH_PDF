# MaenPDF 7.0.1 — Release Notes

## 7.0.1 stabilization
- Fixed Windows QML startup failure caused by unsupported direct `topPadding` on `ColumnLayout`.
- Fixed latent thumbnail delegate failure caused by unsupported direct `bottomPadding` on `Label`.
- Opening a PDF from the command line or Windows file association now switches directly from Home to the document workspace.
- Windows CI now opens a generated one-page PDF fixture and rejects QML runtime property errors, not only Home-screen startup failures.

MaenPDF 7 is the first modular local-first architecture release.

Highlights:

- Complete EN/AR LTR/RTL workspace.
- Multi-document tabs, Home/recent files, dark and low-memory modes.
- Password-protected PDF opening.
- New crop, Bates numbering and secure flattened redaction workflows.
- Local PDF compare, image export, image-to-PDF and safe-flatten tools.
- Optional qpdf, Tesseract OCR and LibreOffice providers discovered at runtime.
- Windows and Android packaging pipelines retained from the stabilized v6 line.
- GPL-3.0-or-later source distribution, contribution guide, security policy, SBOM and separate MaenPDF brand policy.

Capability note: structural editing of existing PDF objects, certificate-backed signing, AcroForm authoring and object-level redaction are not advertised as complete in 7.0.
