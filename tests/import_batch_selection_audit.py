from pathlib import Path

root = Path(__file__).resolve().parents[1]
qml = (root / 'ui/PdfImportDialog.qml').read_text(encoding='utf-8')
cpp = (root / 'core/WebPdfImporter.cpp').read_text(encoding='utf-8')
h = (root / 'core/WebPdfImporter.h').read_text(encoding='utf-8')

checks = {
    'native resizable import window':
        'ApplicationWindow {' in qml
        and 'Qt.WindowMinimizeButtonHint' in qml
        and 'Qt.WindowMaximizeButtonHint' in qml
        and 'Qt.WindowCloseButtonHint' in qml,
    'checkbox multi-selection':
        'CheckBox {' in qml
        and 'selectedUrls' in qml
        and 'selectedCount' in qml
        and 'beginSelected()' in qml,
    'select all and clear selection':
        'function selectAll()' in qml
        and 'function clearSelection()' in qml,
    'background close keeps downloads':
        'closeEvent.accepted = false' in qml
        and 'downloads continue in the background' in qml,
    'better candidate titles':
        'prettyTitle' in cpp
        and 'anchorRx' in cpp
        and 'entry["host"]' in cpp,
    'standards embedded pdf detection':
        'application/pdf' in cpp
        and 'explicitPdf' in cpp,
    'desktop background lifecycle':
        'setQuitOnLastWindowClosed(false)' in cpp
        and 'QSystemTrayIcon' in cpp
        and 'evaluateDesktopLifetime' in cpp
        and 'restoreMainWindow' in cpp,
    'safe web import restrictions preserved':
        'isInSubnet(QHostAddress("10.0.0.0"), 8)' in cpp
        and 'Redirect to restricted URL' in cpp,
    'reset remains invokable':
        'Q_INVOKABLE void resetUi();' in h
        and 'void WebPdfImporter::resetUi()' in cpp,
}

for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL'), name)

raise SystemExit(0 if all(checks.values()) else 1)
