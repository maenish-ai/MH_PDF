# MaenPDF 7.2.1 — Release Checklist

## Interaction and UX
- [x] Continuous page scrolling replaces the single fixed page viewer.
- [x] Pointer selects source PDF text.
- [x] Copy selected text and Highlight Selection are wired.
- [x] Highlight follows text geometry and has a scanned-page area fallback.
- [x] Draw previews the stroke live and commits it with Undo/Redo.
- [x] Crop uses preview, adjustable handles and Apply/Cancel.
- [x] Redaction uses preview and Apply/Cancel.
- [x] Text insertion remains active on click.
- [x] EN/AR catalogs remain in parity and RTL/LTR switching remains complete.
- [x] Toolbar has differentiated tool colors and contextual hints.

## Performance and privacy
- [x] Visible page delegates are virtualized/reused.
- [x] Bounded LRU render cache and Low Memory Mode retained.
- [x] Page and thumbnail images request asynchronous rendering.
- [x] Qt PDF backend access is mutex serialized.
- [x] Long export/provider operations keep the native event loop responsive.
- [x] No document cloud upload path exists in the core.

## Windows
- [x] Version `7.2.1`.
- [x] Clean application-directory replacement on update retained.
- [x] Native Print / Print Current Page / Print Preview retained.
- [x] VC143 runtime remains bundled side-by-side.
- [x] Native startup and real-PDF-open smoke tests retained.
- [x] New interaction-engine smoke test runs before installer generation.

## Android
- [x] Package ID remains `org.orbispdf.app` for update continuity.
- [x] versionName `7.2.1`.
- [x] versionCode `70201`.
- [x] Target API 36 / minimum API 28.
- [x] Signed installable CI APK preserved before AAB build.

## Before public release
- [ ] Confirm the new GitHub Actions run is fully green.
- [ ] Install `MaenPDF-Setup.exe` over an older Windows version and verify the clean upgrade on a real PC.
- [ ] Test wheel scrolling, selection, drawing, crop and redaction with several real PDFs including Arabic text.
- [ ] Test a large (200+ page) PDF and observe RAM/interaction responsiveness.
- [ ] Install the APK on an Android 9/API 28 device and a current Android device.

GitHub Actions is the final native compile/package gate for this source bundle.
