from pathlib import Path
import json,re,sys
root=Path(__file__).resolve().parents[1]
en=json.loads((root/'i18n/en.json').read_text(encoding='utf-8'))
ar=json.loads((root/'i18n/ar.json').read_text(encoding='utf-8'))
qml='\n'.join(p.read_text(encoding='utf-8') for p in (root/'ui').rglob('*.qml'))
allcpp='\n'.join(p.read_text(encoding='utf-8') for p in (root/'core').glob('*.cpp'))
checks={}
checks['same translation keys']=set(en)==set(ar)
checks['English catalog has no Arabic characters']=not any(re.search(r'[\u0600-\u06FF]',v) for v in en.values())
def visible_arabic(v):
    v=re.sub(r'\*\.[A-Za-z0-9]+','',v)
    v=re.sub(r'%\d+','',v)
    v=v.replace('MaenPDF','')
    return v
checks['Arabic catalog has Arabic for every entry']=all(k == 'app.brand' or re.search(r'[\u0600-\u06FF]',visible_arabic(v)) for k,v in ar.items())
checks['Arabic catalog has no stray Latin UI words']=all(not re.search(r'[A-Za-z]{2,}',visible_arabic(v)) for v in ar.values())
checks['MaenPDF brand is exact in both languages']=en.get('app.brand') == 'MaenPDF' and ar.get('app.brand') == 'MaenPDF'
refs=set(re.findall(r'tx\("([^"]+)"',qml))
checks['all QML translation keys exist']=refs <= set(en)
msg=set(re.findall(r'(?:errorOccurred|info|fail|done)\(QStringLiteral\("([^"]+)"\)',allcpp))
checks['all C++ user message keys exist']=msg <= set(en)
visible_literals=[]
for m in re.finditer(r'\b(?:text|title|placeholderText)\s*:\s*"([^"]*)"',qml):
    value=m.group(1)
    if re.search(r'[A-Za-z\u0600-\u06FF]',value): visible_literals.append(value)
checks['no hard-coded visible language in QML']=not visible_literals
checks['English is default']='QStringLiteral("en")' in (root/'core/LanguageManager.h').read_text(encoding='utf-8')
checks['RTL toggles with Arabic']='Qt::RightToLeft' in (root/'core/LanguageManager.cpp').read_text(encoding='utf-8')
checks['large bilingual catalog']=len(en) >= 240
for name,ok in checks.items(): print(('PASS' if ok else 'FAIL'),name)
if visible_literals: print('Hard-coded visible literals:',visible_literals)
if refs-set(en): print('Missing QML keys:',sorted(refs-set(en)))
if msg-set(en): print('Missing C++ keys:',sorted(msg-set(en)))
sys.exit(0 if all(checks.values()) else 1)
