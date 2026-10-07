from pathlib import Path
import re, sys
root = Path(__file__).resolve().parents[1]
qml = (root / 'ui/Main.qml').read_text(encoding='utf-8')
objects = {
    'pdfDocument': root / 'core/PdfDocument.h',
    'pdfTools': root / 'core/PdfToolsService.h',
    'documentManager': root / 'core/DocumentManager.h',
    'printService': root / 'core/PrintService.h',
    'appSettings': root / 'core/AppSettings.h',
    'i18n': root / 'core/LanguageManager.h',
}
checks = {}
for obj, header_path in objects.items():
    header = header_path.read_text(encoding='utf-8')
    calls = sorted(set(re.findall(r'\b' + re.escape(obj) + r'\.([A-Za-z_]\w*)\s*\(', qml)))
    missing = [name for name in calls if re.search(r'\b' + re.escape(name) + r'\s*\(', header) is None]
    checks[f'{obj} QML calls resolve ({len(calls)})'] = not missing
    if missing:
        print(f'Missing methods for {obj}:', missing)
ids = set(re.findall(r'\bid\s*:\s*([A-Za-z_]\w*)', qml))
dialog_refs = set(re.findall(r'\b([A-Za-z_]\w*Dlg)\.(?:open|close|accept|reject)\(', qml))
checks['dialog references resolve'] = not (dialog_refs - ids)
checks['watermark menu is wired'] = 'waterDlg.open()' in qml and 'watermarkDlg.open()' not in qml
checks['provider-dependent document actions explain missing providers'] = qml.count('requireProvider("qpdf")') >= 6 and 'requireProvider("tesseract")' in qml
caps=(root/'core/EngineCapabilities.cpp').read_text(encoding='utf-8')
checks['local forms are wired while native AcroForm authoring stays honest'] = 'chooseTool("formFill")' in qml and 'addFormField' in (root/'core/PdfDocument.h').read_text(encoding='utf-8') and 'capabilities[QStringLiteral("acroFormAuthoring")] = false' in caps
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL'), name)
if dialog_refs - ids:
    print('Undefined dialog references:', sorted(dialog_refs - ids))
sys.exit(0 if all(checks.values()) else 1)
