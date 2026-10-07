# MaenPDF 7.3.1 — Build Status

## Local verification
The source bundle must pass all source-level release gates before packaging:
- source/reliability audit;
- EN/AR localization parity audit;
- release/packaging audit;
- engine v7 architecture audit;
- security audit;
- performance audit;
- interaction audit;
- open-source/brand audit;
- JSON/XML/YAML parsing and manifest verification.

Run `python scripts/preflight.py`.

## 7.3.1 stability/performance changes
- Acrobat-familiar shortcut profile has a dedicated static audit, including text-input focus guards.
- Inserted text deletion is covered by the native interaction smoke test.
- Dialog/action references are audited so menu commands cannot silently point to missing dialog IDs.
- Adaptive RAM profiles select conservative render/cache/undo budgets automatically on 4–6 GB machines.
- Release hardening covers control-flow/DEP-ASLR on MSVC, stack protection/RELRO where supported, plus a Qt image allocation ceiling.
- Incremental and spatially-decimated ink preview; no full stroke-array clone per pointer event.
- Bounded undo history and reduced/quantized render cache.
- Serialized shared render cache for asynchronous image requests.
- Idle-debounced lightweight recovery journal replaces timer-driven full-PDF recovery export.
- Heavy optional local PDF tools have Qt Concurrent background entry points and a busy-state overlay.
- Windows uses accelerated rendering by default with Safe Graphics fallback.
- Windows interaction smoke includes a 2,000-point committed ink timing check.
- Standard CI no longer uploads a Windows Portable artifact.
- Standard Android CI builds/verifies one installable test APK; the production Android Release workflow builds the persistently signed APK and Play AAB.

## Native build authority
This working environment does not include the complete Qt 6.11 Windows and Android kits. GitHub Actions remains the authoritative native compile/package/runtime gate after this source bundle is uploaded. No bundle should be described as GitHub-green until that run succeeds.
