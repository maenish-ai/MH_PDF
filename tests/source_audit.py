from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
cpp=(root/'core/PdfDocument.cpp').read_text(encoding='utf-8')
h=(root/'core/PdfDocument.h').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
qml='\n'.join(p.read_text(encoding='utf-8') for p in (root/'ui').rglob('*.qml'))
main=(root/'app/main.cpp').read_text(encoding='utf-8')
tools=(root/'core/PdfToolsService.cpp').read_text(encoding='utf-8')
qtbackend=(root/'core/QtPdfBackend.h').read_text(encoding='utf-8')
checks={
 'C++ delimiter balance':all((f.read_text(encoding='utf-8').count('{')==f.read_text(encoding='utf-8').count('}') and f.read_text(encoding='utf-8').count('(')==f.read_text(encoding='utf-8').count(')')) for f in list((root/'core').glob('*.cpp'))+list((root/'app').glob('*.cpp'))),
 'safe atomic save':'atomicExportPdf' in cpp and '.maenpdf-backup' in cpp and 'QFile::rename(temp, target)' in cpp,
 'save verification':'check.pageCount() != m_pages.count()' in cpp,
 'security process timeout':'waitForFinished(60000)' in cpp and 'waitForFinished(timeoutMs)' in tools,
 'unique security temp':'QTemporaryFile' in cpp and 'QTemporaryDir' in tools,
 'recovery timer':'m_autosaveTimer.setInterval(60000)' in cpp,
 'capability contract':'EngineCapabilities::current()' in cpp,
 'diagnostic logger':'AppLogger::write' in cpp and 'core/AppLogger.cpp' in cm,
 'lazy page model':'sourceId' in (root/'core/PageModel.h').read_text(encoding='utf-8') and 'setRenderer' in cpp,
 'chunked Android import':'QByteArray buffer(256 * 1024' in cpp,
 'architecture doc':(root/'docs/V7_ARCHITECTURE.md').exists(),
 'undo core operations':all(x in cpp for x in ['Add blank page','Paste page','Combine PDF','Watermark','Page numbers','Bates numbering','Crop page']),
 'QML child objects do not use semicolon separators':re.search(r'\}\s*;\s*[A-Z][A-Za-z0-9_]*\s*\{', qml) is None,
 'QML Label letter spacing uses font group':re.search(r'(?<!font\.)\bletterSpacing\s*:', qml) is None and 'font.letterSpacing:' in qml,
 'QML direct side padding avoided':re.search(r'(?<![A-Za-z0-9_.])(topPadding|bottomPadding|leftPadding|rightPadding)\s*:', qml) is None,
 'QtPdf logical reads are const-safe with Qt 6.11':'mutable QPdfDocument m_doc;' in qtbackend,
 'Windows startup defaults to software Qt Quick backend':'QT_QUICK_BACKEND' in main and 'QByteArrayLiteral("software")' in main and 'QSGRendererInterface::Software' in main,
 'Windows controls style pinned to Basic':'QQuickStyle::setStyle(QStringLiteral("Basic"))' in main,
 'application identity precedes logger install':main.index('QGuiApplication::setApplicationName') < main.index('AppLogger::install()'),
 'Windows startup failure is visible':'MaenPDF Startup Error' in main and 'MessageBoxW' in main,
 'startup cwd normalized to app dir':'QDir::setCurrent(QCoreApplication::applicationDirPath())' in main,
 'CLI PDF opens document workspace':'setProperty("homeVisible", false)' in main and 'documentManager.openDocument' in main,
 'Windows PDF fixture generator':(root/'scripts/create_smoke_pdf.py').exists(),
 'password-protected PDFs supported':'openDocumentWithPassword' in h and 'm_doc.setPassword(password)' in (root/'core/QtPdfBackend.cpp').read_text(encoding='utf-8'),
 'secure flattened redaction implemented':'addRedaction' in h and 'Qt::black' in cpp and 'Secure flattened redaction' in cpp,
 'page crop implemented':'cropPage' in h and 'rendered.copy(crop)' in cpp,
 'Bates numbering implemented':'addBatesNumbers' in h and 'batesText' in (root/'core/PageModel.h').read_text(encoding='utf-8'),
 'multi-document session manager':'DocumentManager' in main and 'documentManager.currentDocument' in qml,
 'real desktop shortcut guard':'onClosing:' in qml and 'documentManager.hasModifiedDocuments' in qml,
}
for name,ok in checks.items(): print(('PASS' if ok else 'FAIL'),name)
sys.exit(0 if all(checks.values()) else 1)
