from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'core/UpdateManager.cpp').read_text()
a=(r/'app/main.cpp').read_text()
assert 'QVersionNumber::compare(newer,current)>0' in s
assert 'lastAutoPromptPair' in s
assert 'installed+QStringLiteral(" -> ")+m_version' in s
assert 'settings.sync()' in s
assert 'emit updateFound();' in s
assert 'setApplicationVersion(' in a
print('Update-loop static regression checks passed')
