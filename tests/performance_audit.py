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
 'PDF image provider stays on GUI-safe synchronous path':'asynchronous: true' not in qml and qml.count('asynchronous: false') >= 2,
 'QML images bypass duplicate scene cache':'cache: false' in qml,
 'optional tools discovered at runtime':'QStandardPaths::findExecutable' in tools,
 'optional tools not linked into core build':'find_package(Tesseract' not in (r/'CMakeLists.txt').read_text(encoding='utf-8') and 'find_package(qpdf' not in (r/'CMakeLists.txt').read_text(encoding='utf-8'),
}
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
