from pathlib import Path
import sys,shutil
src=Path(__file__).resolve().parent
repo=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path.cwd()
cm=repo/'CMakeLists.txt'; app=repo/'app/main.cpp'; ui=repo/'ui/Main.qml'
for p in (cm,app,ui):
 if not p.is_file(): raise SystemExit(f'Missing {p}')
c=cm.read_text();a=app.read_text();q=ui.read_text()
assert 'COMPONENTS Core Gui Quick Qml QuickControls2 Pdf Concurrent' in c
assert 'core/PrintService.cpp core/PrintService.h' in c
assert 'Qt6::Pdf Qt6::Concurrent' in c
assert 'PrintService printService;' in a
assert 'setContextProperty(QStringLiteral("printService"), &printService);' in a
assert q.rstrip().endswith('}')
c=c.replace('COMPONENTS Core Gui Quick Qml QuickControls2 Pdf Concurrent','COMPONENTS Core Gui Quick Qml QuickControls2 Pdf Concurrent Network').replace('core/PrintService.cpp core/PrintService.h','core/PrintService.cpp core/PrintService.h\n    core/WebPdfImporter.cpp core/WebPdfImporter.h').replace('QML_FILES ui/Main.qml','QML_FILES ui/Main.qml ui/PdfImportDialog.qml').replace('Qt6::Pdf Qt6::Concurrent','Qt6::Pdf Qt6::Concurrent Qt6::Network')
a=a.replace('#include "../core/PrintService.h"','#include "../core/PrintService.h"\n#include "../core/WebPdfImporter.h"').replace('PrintService printService;','PrintService printService;\n    WebPdfImporter webPdfImporter;').replace('setContextProperty(QStringLiteral("printService"), &printService);','setContextProperty(QStringLiteral("printService"), &printService);\n    engine.rootContext()->setContextProperty(QStringLiteral("webPdfImporter"), &webPdfImporter);')
q=q.rstrip()[:-1]+'''\n    Shortcut { sequence: "Ctrl+Shift+I"; onActivated: pdfImportDialog.open() }\n    PdfImportDialog {\n        id: pdfImportDialog\n        parent: Overlay.overlay\n        importer: webPdfImporter\n        documents: documentManager\n    }\n}\n'''
for name in ('core/WebPdfImporter.h','core/WebPdfImporter.cpp','ui/PdfImportDialog.qml'):
 dest=repo/name
 if dest.exists(): raise SystemExit('Already exists: '+str(dest))
 dest.parent.mkdir(parents=True,exist_ok=True)
 shutil.copy2(src/name,dest)
for p,v in ((cm,c),(app,a),(ui,q)):p.write_text(v)
print('Applied; inspect git diff and build with Qt 6.11')
