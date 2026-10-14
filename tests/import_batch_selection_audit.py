from pathlib import Path
root=Path(__file__).resolve().parents[1]
qml=(root/'ui/PdfImportDialog.qml').read_text()
cpp=(root/'core/WebPdfImporter.cpp').read_text()
h=(root/'core/WebPdfImporter.h').read_text()
for value in ['selectedIndex','resetDialog()', 'dialog.beginAll()', 'dialog.selectedIndex === index', 'web_pdf.download_selected','web_pdf.reset','importer.resetUi()']:
    assert value in qml, value
assert 'Q_INVOKABLE void resetUi();' in h
assert 'm_reply=nullptr;' in cpp and 'void WebPdfImporter::resetUi()' in cpp
for lang in ['en','ar']:
    import json
    d=json.loads((root/'i18n'/f'{lang}.json').read_text())
    assert all(k in d for k in ['web_pdf.reset','web_pdf.download_selected'])
print('PASS: reset and selection source contract')
