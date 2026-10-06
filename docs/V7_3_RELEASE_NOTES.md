# MaenPDF 7.3.0 — Performance & Professional Tools

- Reworked live ink so pointer movement no longer clones and redraws the full stroke on every event.
- Added point decimation and threaded incremental Canvas painting.
- Bounded undo history and reduced overlay/cache budgets for old hardware.
- Drawing undo now stores only the touched pixel patch instead of retaining a full-page overlay bitmap for every stroke.
- Quantized render sizes to prevent zoom-cache explosion.
- Serialized the shared render cache.
- Recovery now uses an idle-debounced lightweight local journal instead of full-PDF timer exports during active work.
- Optional heavy local tools now have background-job entry points with a busy overlay so the main UI remains responsive.
- Windows uses accelerated Qt Quick rendering by default, with persistent Safe Graphics mode and startup-failure fallback.
- Standard CI no longer uploads a Windows Portable artifact and no longer spends time building an AAB on every push.
- Manual Android Release workflow now preserves a persistently signed release APK before building the Play AAB.
- Added long-stroke interaction timing to Windows CI.
- Added 2026 feature/user-pain matrix for roadmap discipline.

- Added qpdf-backed batch optimization and a privacy/sanitize flatten entry point.
