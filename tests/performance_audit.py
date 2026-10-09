from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
pm=(r/'core/PageModel.cpp').read_text(encoding='utf-8')
mem=(r/'core/MemoryPolicy.cpp').read_text(encoding='utf-8')
settings=(r/'core/AppSettings.cpp').read_text(encoding='utf-8')
qml=(r/'ui/Main.qml').read_text(encoding='utf-8')
tools=(r/'core/PdfToolsService.cpp').read_text(encoding='utf-8')
checks={
 'bounded LRU cache':'while (!m_lru.isEmpty() && m_cacheBytes + bytes > m_cacheBudget)' in pm,
 'large image not cached':'bytes > m_cacheBudget / 2' in pm,
 'physical RAM detection':'GlobalMemoryStatusEx' in mem and '_SC_PHYS_PAGES' in mem,
 'adaptive performance profile':'performanceProfile()' in mem and 'performance/adaptive' in mem,
 '4GB desktop cache bounded':'return qint64(32) * 1024 * 1024' in mem,
 '4GB render dimensions bounded':'memory <= 4096) return 1600' in mem,
 'adaptive UI default':'m_adaptivePerformance = settings.value(QStringLiteral("performance/adaptive"), true)' in settings,
 'render dimensions bounded':'maxRenderDimension' in pm and 'qBound' in pm,
 'PDF pages load asynchronously':qml.count('asynchronous: true') >= 2 and 'BusyIndicator {' in qml,
 'QML images bypass duplicate scene cache':'cache: false' in qml,
 'continuous viewer virtualizes delegates':'reuseItems: true' in qml and 'cacheBuffer: zoomRenderTimer.running ? 0' in qml and 'performanceProfile' in qml,
 'provider document access serialized':'QMutexLocker' in (r/'core/QtPdfBackend.cpp').read_text(encoding='utf-8'),
 'local provider work has async wrappers':'QtConcurrent::run' in tools and 'beginAsync' in tools,
 'async render uses page snapshot':'const PageItem page = m_pages.at(row);' in pm,
 'render cache is serialized':'QMutexLocker locker(&m_cacheMutex)' in pm,
 'zoom requests are quantized':'const int bucket = MemoryPolicy::performanceProfile()' in pm and '? 128' in pm and '? 96 : 64' in pm,
 'zoom render is debounced':'id: zoomRenderTimer' in qml and 'renderZoom' in qml and 'zoomRenderTimer.restart()' in qml,
 'single page edit does not invalidate every image':'source: pageImage' in qml and 'var modelRev = pdfDocument.pages.modelRevision\n                                    return pageImage' not in qml,
 'idle debounced recovery':'m_autosaveTimer.setSingleShot(true)' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and '120000' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'each edit restarts idle recovery':'every edit restarts the idle-recovery debounce' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'lightweight recovery journal':'MaenPDF-Recovery-1' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'Recovery journal updated' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'QJsonDocument(root).toJson' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'incremental live ink':'paintedInkPoints' in qml and 'localInkPoints.slice(0)' not in qml,
 'threaded live ink canvas':'renderStrategy: Canvas.Threaded' in qml,
 'unlimited undo history':'setUndoLimit(MemoryPolicy::undoLimit())' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'return 0;' in mem,
 'draw undo stores touched patch':'beforePatch = page->overlay.copy(patchRect)' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and 'CompositionMode_Source' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
 'highlight/redaction undo stores touched patches':'pixelBounds' in (r/'core/PdfDocument.cpp').read_text(encoding='utf-8') and (r/'core/PdfDocument.cpp').read_text(encoding='utf-8').count('beforePatch = page->overlay.copy') >= 3,
 'inserted text avoids page bitmap copies':'TextOverlayItem' in (r/'core/PageModel.h').read_text(encoding='utf-8') and 'page.textItems' in pm,
 'closed tabs are retired after in-flight renders': 'QTimer::singleShot(30000' in (r/'core/DocumentManager.cpp').read_text(encoding='utf-8'),
 'optional tools discovered at runtime':'QStandardPaths::findExecutable' in tools,
 'optional tools not linked into core build':'find_package(Tesseract' not in (r/'CMakeLists.txt').read_text(encoding='utf-8') and 'find_package(qpdf' not in (r/'CMakeLists.txt').read_text(encoding='utf-8'),
}
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
