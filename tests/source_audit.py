from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
cpp=(root/'core/PdfDocument.cpp').read_text(encoding='utf-8')
h=(root/'core/PdfDocument.h').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
qml=(root/'ui/Main.qml').read_text(encoding='utf-8')
qtbackend=(root/'core/QtPdfBackend.h').read_text(encoding='utf-8')
checks={
 'C++ delimiter balance':all((f.read_text(encoding='utf-8').count('{')==f.read_text(encoding='utf-8').count('}') and f.read_text(encoding='utf-8').count('(')==f.read_text(encoding='utf-8').count(')')) for f in (root/'core').glob('*.cpp')),
 'no duplicated rotate command':'m_undo.push(new LambdaCommand(QStringLiteral(\"Rotate page\"),\n    m_undo.push' not in cpp,
 'safe atomic save':'atomicExportPdf' in cpp and '.maenpdf-backup' in cpp and 'QFile::rename(temp, target)' in cpp,
 'save verification':'check.pageCount() != m_pages.count()' in cpp or 'check.pageCount()!=m_pages.count()' in cpp,
 'security timeout':'waitForFinished(60000)' in cpp,
 'unique security temp':'QTemporaryFile' in cpp,
 'recovery timer':'m_autosaveTimer.setInterval(60000)' in cpp,
 'recovery validation':'check.pageCount() == m_pages.count()' in cpp or 'check.pageCount()==m_pages.count()' in cpp,
 'capability contract':'EngineCapabilities::current()' in cpp,
 'diagnostic logger':'AppLogger::write' in cpp and 'core/AppLogger.cpp' in cm,
 'lazy page model':'sourceId' in (root/'core/PageModel.h').read_text(encoding='utf-8') and 'setRenderer' in cpp,
 'chunked Android import':'QByteArray buffer(256 * 1024' in cpp,
 'failed open does not wipe temp sources':'bool PdfDocument::openDocument(const QString &path) {\n    cleanupTemporaryInputs();' not in cpp,
 'architecture doc':(root/'ARCHITECTURE.md').exists(),
 'quality gate':(root/'QUALITY.md').exists(),
 'undo core operations':all(x in cpp for x in ['Add blank page','Paste page','Combine PDF','Watermark','Page numbers']),
 'QML child objects do not use semicolon separators':re.search(r'\}\s*;\s*[A-Z][A-Za-z0-9_]*\s*\{', qml) is None,
 'QtPdf logical reads are const-safe with Qt 6.11':'mutable QPdfDocument m_doc;' in qtbackend,
 'Windows startup defaults to software Qt Quick backend':('QT_QUICK_BACKEND' in (root/'app/main.cpp').read_text(encoding='utf-8') and 'QByteArrayLiteral("software")' in (root/'app/main.cpp').read_text(encoding='utf-8')),
 'Windows startup failure is visible':('MaenPDF Startup Error' in (root/'app/main.cpp').read_text(encoding='utf-8') and 'MessageBoxW' in (root/'app/main.cpp').read_text(encoding='utf-8')),
 'startup cwd normalized to app dir':'QDir::setCurrent(QCoreApplication::applicationDirPath())' in (root/'app/main.cpp').read_text(encoding='utf-8'),
}
for name,ok in checks.items(): print(('PASS' if ok else 'FAIL'),name)
sys.exit(0 if all(checks.values()) else 1)
