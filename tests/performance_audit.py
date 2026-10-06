from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
pm=(r/'core/PageModel.cpp').read_text(encoding='utf-8')
mem=(r/'core/MemoryPolicy.cpp').read_text(encoding='utf-8')
qml=(r/'ui/Main.qml').read_text(encoding='utf-8')
tools=(r/'core/PdfToolsService.cpp').read_text(encoding='utf-8')
checks={
 'bounded LRU cache':'while (!m_lru.isEmpty() && m_cacheBytes + bytes > m_cacheBudget)' in pm,
 'large image not cached':'bytes > m_cacheBudget / 2' in pm,
 'low-memory desktop budget':'lowMemory ? 48 : 128' in mem,
 'low-memory Android budget':'lowMemory ? 32 : 72' in mem,
 'render dimensions bounded':'maxRenderDimension' in pm and 'qBound' in pm,
 'PDF pages load asynchronously':qml.count('asynchronous: true') >= 2 and 'BusyIndicator {' in qml,
 'QML images bypass duplicate scene cache':'cache: false' in qml,
 'continuous viewer virtualizes delegates':'reuseItems: true' in qml and 'cacheBuffer:' in qml,
 'provider document access serialized':'QMutexLocker' in (r/'core/QtPdfBackend.cpp').read_text(encoding='utf-8'),
 'local provider work has async wrappers':'QtConcurrent::run' in tools and 'beginAsync' in tools,
 'async render uses page snapshot':'const PageItem page = m_pages.at(row);' in pm,
 'render cache is serialized':'QMutexLocker locker(&m_cacheMutex)' in pm,
 'zoom requests are quantized':'const int bucket = 48' in pm,
 'idle debounced recovery':'m_autosaveTimer.setSingleShot(true)' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and '120000' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'lightweight recovery journal':'MaenPDF-Recovery-1' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'Recovery journal updated' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'QJsonDocument(root).toJson' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'incremental live ink':'paintedInkPoints' in qml and 'localInkPoints.slice(0)' not in qml,
 'threaded live ink canvas':'renderStrategy: Canvas.Threaded' in qml,
 'bounded undo':'setUndoLimit(MemoryPolicy::undoLimit())' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'draw undo stores touched patch':'beforePatch = page->overlay.copy(patchRect)' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'CompositionMode_Source' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'closed tabs are retired after in-flight renders': 'QTimer::singleShot(30000' in (r/'core/DocumentManager.cpp').read_text(encoding='utf-8'),
 'optional tools discovered at runtime':'QStandardPaths::findExecutable' in tools,
 'optional tools not linked into core build':'find_package(Tesseract' not in (r/'CMakeLists.txt').read_text(encoding='utf-8') and 'find_package(qpdf' not in (r/'CMakeLists.txt').read_text(encoding='utf-8'),
}
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
