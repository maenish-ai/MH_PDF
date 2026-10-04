# Orbis PDF Professional Engine 6

## Implemented
- Replaceable `IPdfBackend` reader boundary with Qt PDF backend.
- Source-backed page records and lazy rendering.
- Bounded LRU page-image cache with conservative Android policy.
- Search of source PDF text with page/excerpt results.
- Standard PDF export and safe verified replacement.
- Page operations and overlay editing with undo/redo coverage for core actions.
- Autosave recovery and diagnostics.
- Android content URI import/export bridge.
- EN/AR interface architecture with RTL.

## Capability boundary
The current writer flattens edited page appearance on export. Structural content-stream editing, OCR, certificate-backed signatures, AcroForm authoring and true redaction remain false in `EngineCapabilities` until dedicated providers are integrated and regression-tested.

## Performance contract
Opening a many-page document must not call the backend renderer for every page. Rendered pages are cached only within the configured memory budget and are evicted by LRU order. Set `ORBIS_CACHE_MB` on desktop during profiling to override the default cache budget within the accepted range.
