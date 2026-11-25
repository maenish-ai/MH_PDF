# MaenPDF 7 Software Bill of Materials (human-readable)

MaenPDF 7 is built from a small core and optional local providers.

| Component | Role | Required | License / distribution note |
| --- | --- | --- | --- |
| Qt 6.11.2 Core/Gui/Quick/Qml/QuickControls2/Pdf | UI, rendering, PDF reading | Yes | Use under the applicable Qt open-source license obligations for your distribution |
| Qt PDF / PDFium integration | PDF rendering and text extraction | Yes | Distributed through Qt PDF; review Qt licensing documentation for release obligations |
| Microsoft VC143 runtime | Windows runtime | Windows build | Redistributed from the Visual Studio redist directory by CI |
| qpdf | Encryption, optimization, repair, split, merge and OCR page assembly | Windows OCR bundle | Apache-2.0; UCRT64 binary from MSYS2 |
| Tesseract OCR and Arabic/English tessdata_fast | Local OCR and searchable PDF output | Windows OCR bundle | Apache-2.0; UCRT64 engine and trained models from MSYS2 |
| LibreOffice | Office-document to PDF bridge | Optional | External local executable; not bundled by MaenPDF |
| Inno Setup | Windows installer compiler | CI only | Used only while building the installer |

The Windows installer ships Tesseract, Arabic and English language data, qpdf,
and their native runtime dependencies under `tools/`. The installer records
the MSYS2 package versions and their packaged license notices in
`tools/ocr-notices/`. The [MSYS2 package recipes](https://github.com/msys2/MINGW-packages)
identify the corresponding upstream sources. LibreOffice remains external.
On other platforms the application discovers compatible locally installed OCR providers.
