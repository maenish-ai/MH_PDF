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
## GitHub Actions run #3 diagnosis (2026-10-04)

The third hosted build confirmed that the Windows generator/toolchain is now correct (MSVC 2022) and QtPdf installs successfully. Two later blockers were identified and fixed in this bundle:

- Windows: `QPdfDocument::render()` and `getAllText()` are non-const in Qt 6.11. The backend now keeps the document `mutable` so logically read-only backend operations can use Qt's internal caches without discarding the const backend API.
- Android: Qt cross-compilation now passes `QT_HOST_PATH` to the host desktop Qt installed by `aqt --autodesktop`, and validates that host Qt before CMake configure.

These fixes are also guarded by the source/release audits. The next GitHub Actions run remains the authoritative native compile/package check.
