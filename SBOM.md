# MaenPDF 7 Software Bill of Materials (human-readable)

MaenPDF 7 is built from a small core and optional local providers.

| Component | Role | Required | License / distribution note |
| --- | --- | --- | --- |
| Qt 6.11.2 Core/Gui/Quick/Qml/QuickControls2/Pdf | UI, rendering, PDF reading | Yes | Use under the applicable Qt open-source license obligations for your distribution |
| Qt PDF / PDFium integration | PDF rendering and text extraction | Yes | Distributed through Qt PDF; review Qt licensing documentation for release obligations |
| Microsoft VC143 runtime | Windows runtime | Windows build | Redistributed from the Visual Studio redist directory by CI |
| qpdf | Encryption, optimization, repair, split, merge support | Optional | External local executable; not bundled by the core source package |
| Tesseract OCR | Local OCR/searchable scanned PDF workflow | Optional | External local executable; language data installed separately |
| LibreOffice | Office-document to PDF bridge | Optional | External local executable; not bundled by MaenPDF |
| Inno Setup | Windows installer compiler | CI only | Used only while building the installer |

The project intentionally keeps optional providers external so the base application remains small and usable when they are absent.
