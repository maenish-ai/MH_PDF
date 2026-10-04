# Orbis PDF 6.0 — Build Status

Local release gates in this source bundle pass:
- source/reliability audit;
- EN/AR localization audit;
- packaging/release audit;
- engine-v6 architecture audit;
- JSON/XML/YAML syntax parsing.

The current execution environment does not contain the Qt 6.11 SDK, so a native Windows/Android compilation cannot be truthfully certified here. GitHub Actions is therefore the authoritative compile/package gate after upload. The workflows pin the required toolchain and will stop before packaging if an audit fails.

Run locally before every push:

```bash
python scripts/preflight.py
```
