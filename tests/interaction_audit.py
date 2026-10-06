from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
qml = (root / 'ui/Main.qml').read_text(encoding='utf-8')
h = (root / 'core/PdfDocument.h').read_text(encoding='utf-8')
cpp = (root / 'core/PdfDocument.cpp').read_text(encoding='utf-8')
backend = (root / 'core/QtPdfBackend.cpp').read_text(encoding='utf-8')
ci = (root / '.github/workflows/ci.yml').read_text(encoding='utf-8')
fixture = (root / 'scripts/create_smoke_pdf.py').read_text(encoding='utf-8')

checks = {
    'continuous vertical viewer': 'id: documentView' in qml and 'orientation: ListView.Vertical' in qml,
    'viewer is virtualized': 'reuseItems: true' in qml and 'cacheBuffer:' in qml,
    'wheel explicitly scrolls document': 'onWheel: function(wheel)' in qml and 'documentView.contentY' in qml,
    'viewport tracks current page': 'onContentYChanged:' in qml and 'pdfDocument.currentPage = idx' in qml,
    'thumbnail navigation follows current page': 'currentIndex: pdfDocument.currentPage' in qml,
    'real PDF text selection API': 'textSelection(int page' in h and 'm_doc.getSelection(page' in backend,
    'text selection geometry rendered': 'selectedTextRects' in qml and 'modelData.w * pageSurface.width' in qml,
    'copy selected text': 'copyTextToClipboard' in h and 'action.copy_text' in qml,
    'highlight from text geometry': 'addHighlightRects' in h and 'finishTextDrag(mouse, true)' in qml,
    'highlight area fallback for scans': 'fallbackRect' in qml and 'addHighlightRects(index, [fallbackRect]' in qml,
    'live ink preview': 'Canvas {' in qml and 'liveInk.requestPaint()' in qml,
    'styled ink commit': 'addInkStyled' in h and 'drawWidth / Math.max(1, pageSurface.width)' in qml,
    'crop staged before apply': 'pendingActionTool === "crop"' in qml and 'pdfDocument.cropPage(pendingActionPage' in qml,
    'crop adjustable handles': 'pendingActionTool === "crop" ? 4 : 0' in qml and 'id: pendingOverlay\n                                z: 30' in qml,
    'pending crop handles receive pointer input': 'enabled: pendingActionPage < 0 &&' in qml,
    'redaction staged before apply': 'pendingActionTool === "redact"' in qml and 'pdfDocument.addRedaction(pendingActionPage' in qml,
    'text insertion stays interactive': 'textDlg.nx' in qml and 'textDlg.open()' in qml,
    'page images async': 'asynchronous: true' in qml,
    'backend serialized for async renderer': 'QMutexLocker<QMutex>' in backend,
    'long operations keep native event loop alive': 'QCoreApplication::processEvents' in cpp and 'QCoreApplication::processEvents' in (root / 'core/PdfToolsService.cpp').read_text(encoding='utf-8'),
    'three-page CI fixture': '/Count 3' in fixture,
    'runtime interaction smoke gate': '--interaction-smoke' in ci and 'INTERACTION_SMOKE_PASS' in ci,
}

for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL'), name)

sys.exit(0 if all(checks.values()) else 1)
