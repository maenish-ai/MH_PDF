#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys
root=Path(__file__).resolve().parents[1]
tests=[
    'source_audit.py',
    'localization_audit.py',
    'release_audit.py',
    'engine_v7_audit.py',
    'security_audit.py',
    'performance_audit.py',
    'interaction_audit.py',
    'shortcuts_audit.py',
    'ui_wiring_audit.py',
    'open_source_audit.py',
]
for test in tests:
    print(f'\n== {test} ==')
    result=subprocess.run([sys.executable,str(root/'tests'/test)],cwd=root)
    if result.returncode:
        raise SystemExit(result.returncode)
print('\nAll MaenPDF v7 local release gates passed.')
