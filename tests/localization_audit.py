from pathlib import Path
import json,re,sys
root=Path(__file__).resolve().parents[1]
en=json.loads((root/'i18n/en.json').read_text(encoding='utf-8'))
ar=json.loads((root/'i18n/ar.json').read_text(encoding='utf-8'))
qml=(root/'ui/Main.qml').read_text(encoding='utf-8')
cpp=(root/'core/PdfDocument.cpp').read_text(encoding='utf-8')
checks={}
checks['same translation keys']=set(en)==set(ar)
checks['English catalog has no Arabic characters']=not any(re.search(r'[\u0600-\u06FF]',v) for v in en.values())
# Remove file-extension glob tokens and % placeholders before enforcing no Latin UI words in Arabic catalog.
def visible_arabic(v):
    v=re.sub(r'\*\.[A-Za-z0-9]+','',v)
    v=re.sub(r'%\d+','',v)
    return v
checks['Arabic catalog has Arabic for every entry']=all(re.search(r'[\u0600-\u06FF]',visible_arabic(v)) for v in ar.values())
checks['Arabic catalog has no stray Latin UI words']=all(not re.search(r'[A-Za-z]{2,}',visible_arabic(v)) for v in ar.values())
# All tx() references must exist.
refs=set(re.findall(r'tx\("([^"]+)"',qml))
checks['all QML translation keys exist']=refs <= set(en)
# All emitted message keys must exist.
msg=set(re.findall(r'(?:errorOccurred|info)\(QStringLiteral\("([^"]+)"\)',cpp))
checks['all C++ user message keys exist']=msg <= set(en)
# Visible literal properties may contain only symbols/empty strings; actual words must use tx().
visible_literals=[]
for m in re.finditer(r'\b(?:text|title|placeholderText)\s*:\s*"([^"]*)"',qml):
    value=m.group(1)
    if re.search(r'[A-Za-z\u0600-\u06FF]',value): visible_literals.append(value)
checks['no hard-coded visible language in QML']=not visible_literals
checks['English is default']='QStringLiteral("en")' in (root/'core/LanguageManager.h').read_text(encoding='utf-8')
checks['RTL toggles with Arabic']='Qt::RightToLeft' in (root/'core/LanguageManager.cpp').read_text(encoding='utf-8')
for name,ok in checks.items():
    print(('PASS' if ok else 'FAIL'),name)
if visible_literals: print('Hard-coded visible literals:',visible_literals)
sys.exit(0 if all(checks.values()) else 1)
