from pathlib import Path
import re, sys

root = Path(__file__).resolve().parents[1]
sources = list((root / 'core').rglob('*.cpp')) + list((root / 'core').rglob('*.h')) + list((root / 'app').rglob('*.cpp')) + list((root / 'app').rglob('*.h'))

# QVariant conversion functions take a bool* success parameter, not a fallback
# value. A negative/numeric/string argument is therefore a compile error or a
# misleading null-pointer literal. Defaults belong in QVariantMap::value(key,
# fallback). QJsonValue conversions are intentionally excluded because their
# API does accept default values.
patterns = {
    'QVariant-style toInt fallback misuse': re.compile(r'\.toInt\(\s*-[0-9]+\s*\)'),
    'QVariant-style toUInt fallback misuse': re.compile(r'\.toUInt\(\s*[1-9][0-9]*\s*\)'),
    'QVariant-style toLongLong fallback misuse': re.compile(r'\.toLongLong\(\s*-?[1-9][0-9]*\s*\)'),
    'QVariant-style toULongLong fallback misuse': re.compile(r'\.toULongLong\(\s*[1-9][0-9]*\s*\)'),
}

violations = []
for path in sources:
    text = path.read_text(encoding='utf-8')
    # The recovery parser uses QJsonValue, where toInt(default) is valid. We
    # only flag suspicious uses in lines whose receiver isn't clearly a QJson
    # object value. Negative toInt is the concrete regression from GitHub Run.
    for lineno, line in enumerate(text.splitlines(), 1):
        for name, rx in patterns.items():
            if rx.search(line):
                # QJsonValue default conversions are valid and appear in the
                # recovery parser as textObject/formObject/item/root.value().
                if path.name == 'PdfDocument.cpp' and any(token in line for token in ('textObject.value(', 'formObject.value(', 'root.value(')):
                    continue
                if path.name == 'PdfDocument.cpp' and 'item.value(' in line and 790 <= lineno <= 890:
                    continue
                violations.append(f'{path.relative_to(root)}:{lineno}: {name}: {line.strip()}')

checks = {
    'no QVariant default-argument conversion misuse': not violations,
    'text annotation default is applied at map lookup': 'item.value(QStringLiteral("strikeStart"), -1).toInt()' in (root/'core/PdfDocument.cpp').read_text(encoding='utf-8'),
}
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL'), name)
if violations:
    print('\n'.join(violations))
sys.exit(0 if all(checks.values()) else 1)
