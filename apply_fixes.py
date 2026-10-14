#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
main=root/'ui/Main.qml'
if not main.exists():
    raise SystemExit('Run: python apply_fixes.py PATH_TO_MH_PDF_REPOSITORY')
text=main.read_text(encoding='utf-8')
old='Label { text: updateManager.available ? tx("updater.available", [updateManager.version]) : tx("updater.none"); wrapMode: Text.Wrap; Layout.fillWidth: true }'
new='''Label {
                text: updateManager.ready
                      ? "Update v" + updateManager.version + " downloaded and verified. Install it, then reopen MaenPDF to activate the new version."
                      : (updateManager.available ? tx("updater.available", [updateManager.version]) : "You're using the latest installed version.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }'''
if old not in text and new not in text:
    raise SystemExit('Main.qml changed; refusing unsafe patch.')
text=text.replace(old,new)
text=text.replace('Button { text: tx("updater.install"); visible: updateManager.ready; onClicked: updateManager.install() }',
                  'Button { text: "Install Update"; visible: updateManager.ready; enabled: !updateManager.busy; onClicked: updateManager.install() }')
main.write_text(text,encoding='utf-8')
dest=root/'ui/PdfImportDialog.qml'
dest.write_bytes((Path(__file__).parent/'ui/PdfImportDialog.qml').read_bytes())
print('Updated:', main, dest)
