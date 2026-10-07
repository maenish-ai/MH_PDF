from pathlib import Path
import re, sys
root = Path(__file__).resolve().parents[1]
qml = (root / 'ui/Main.qml').read_text(encoding='utf-8')
doc = (root / 'docs/KEYBOARD_SHORTCUTS.md').read_text(encoding='utf-8') if (root / 'docs/KEYBOARD_SHORTCUTS.md').exists() else ''
ids = set(re.findall(r'\bid\s*:\s*([A-Za-z_]\w*)', qml))
dialog_refs = set(re.findall(r'\b([A-Za-z_]\w*Dlg)\.(?:open|close|accept|reject)\(', qml))
checks = {
    'shortcut documentation exists': bool(doc),
    'Acrobat zoom in Ctrl+=': 'sequence: ["Ctrl+=", "Ctrl++"]' in qml or 'sequences: ["Ctrl+=", "Ctrl++"]' in qml,
    'zoom out Ctrl+-': 'sequence: "Ctrl+-"' in qml,
    'fit page Ctrl+0': 'sequence: "Ctrl+0"' in qml,
    'actual size Ctrl+1': 'sequence: "Ctrl+1"' in qml,
    'fit width Ctrl+2': 'sequence: "Ctrl+2"' in qml,
    'fullscreen Ctrl+L': 'sequence: "Ctrl+L"' in qml,
    'preferences Ctrl+K': 'sequence: "Ctrl+K"' in qml and 'preferencesDlg.open()' in qml,
    'go to page Ctrl+Shift+N': 'sequence: "Ctrl+Shift+N"' in qml,
    'delete page Ctrl+Shift+D': 'sequence: "Ctrl+Shift+D"' in qml,
    'blank page Ctrl+Shift+T': 'sequence: "Ctrl+Shift+T"' in qml,
    'insert file Ctrl+Shift+I': 'sequence: "Ctrl+Shift+I"' in qml,
    'close document Ctrl+W': 'sequence: "Ctrl+W"' in qml and 'requestCloseTab' in qml,
    'document tabs Ctrl+Tab': 'sequence: "Ctrl+Tab"' in qml and 'sequence: "Ctrl+Shift+Tab"' in qml,
    'pages sidebar F4': 'sequence: "F4"' in qml,
    'single key V/H/T/U/D/C': all(f'sequence: "{key}"' in qml for key in ['V','H','T','U','D','C']),
    'redact Shift+Y': 'sequence: "Shift+Y"' in qml,
    'page navigation shortcuts': all(x in qml for x in ['"PageDown"','"PageUp"','"Ctrl+PageDown"','"Ctrl+PageUp"','"Home"','"End"']),
    'delete and backspace remove selected object': 'sequence: "Delete"' in qml and 'sequence: "Backspace"' in qml and 'deleteCurrentSelection()' in qml,
    'text inputs keep editing keys': 'function keyboardTextInputActive()' in qml and qml.count('enabled: !keyboardTextInputActive()') >= 7,
    'single key shortcuts optional': 'appSettings.singleKeyShortcuts' in qml,
    'command palette has separate shortcut': 'sequence: "Ctrl+Shift+P"' in qml,
    'Ctrl mouse wheel zoom': 'wheel.modifiers & Qt.ControlModifier' in qml,
    'no undefined dialog ids': not (dialog_refs - ids),
    'inserted text becomes immediately deletable': 'tool = "select"' in qml and 'selectedAnnotationId = createdId' in qml,
    'watermark menu points to real dialog': 'waterDlg.open()' in qml and 'watermarkDlg.open()' not in qml,
}
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL'), name)
if dialog_refs - ids:
    print('Undefined dialog references:', sorted(dialog_refs - ids))
sys.exit(0 if all(checks.values()) else 1)
