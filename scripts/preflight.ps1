$ErrorActionPreference = "Stop"
python tests/source_audit.py
python tests/localization_audit.py
python tests/release_audit.py
python tests/engine_v7_audit.py
python tests/security_audit.py
python tests/performance_audit.py
python tests/open_source_audit.py
Write-Host "All MaenPDF v7 local release gates passed."
