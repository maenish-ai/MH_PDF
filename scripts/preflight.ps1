$ErrorActionPreference = "Stop"
python tests/source_audit.py
python tests/localization_audit.py
python tests/release_audit.py
python tests/engine_v6_audit.py
Write-Host "All MaenPDF v6 local release gates passed."
