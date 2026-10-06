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
 'low-memory desktop budget':'lowMemory ? 64 : 192' in mem,
 'low-memory Android budget':'lowMemory ? 48 : 96' in mem,
 'render dimensions bounded':'maxRenderDimension' in pm and 'qBound' in pm,
 'PDF pages load asynchronously':qml.count('asynchronous: true') >= 2 and 'BusyIndicator {' in qml,
 'QML images bypass duplicate scene cache':'cache: false' in qml,
 'continuous viewer virtualizes delegates':'reuseItems: true' in qml and 'cacheBuffer:' in qml,
 'provider document access serialized':'QMutexLocker' in (r/'core/QtPdfBackend.cpp').read_text(encoding='utf-8'),
 'long local provider waits keep event loop responsive':'QCoreApplication::processEvents' in tools,
 'async render uses page snapshot':'const PageItem page = m_pages.at(row);' in pm,
 'closed tabs are retired after in-flight renders': 'QTimer::singleShot(30000' in (r/'core/DocumentManager.cpp').read_text(encoding='utf-8'),
 'optional tools discovered at runtime':'QStandardPaths::findExecutable' in tools,
 'optional tools not linked into core build':'find_package(Tesseract' not in (r/'CMakeLists.txt').read_text(encoding='utf-8') and 'find_package(qpdf' not in (r/'CMakeLists.txt').read_text(encoding='utf-8'),
}
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
