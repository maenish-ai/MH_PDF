# MaenPDF 7.1.0 — Release Checklist

## Source and UX
- [x] Product name remains `MaenPDF`.
- [x] English and Arabic catalogs contain identical keys.
- [x] English mode is LTR and Arabic mode is RTL.
- [x] Professional menu surface is present.
- [x] Searchable Ctrl+K command palette is present.
- [x] Unsupported structural forms/signatures remain disabled instead of falsely advertised as complete.

## Performance and privacy
- [x] Lazy source-backed PDF loading retained.
- [x] Bounded LRU render cache retained.
- [x] Low-memory mode retained.
- [x] No document cloud upload path in the core.
- [x] Optional qpdf/Tesseract/LibreOffice providers remain runtime-discovered.

## Windows
- [x] Version `7.1.0`.
- [x] Clean application-directory replacement on update.
- [x] One-time pre-7.1 settings/cache migration.
- [x] Optional installer reset-settings task.
- [x] Native Print, Print Current Page and Print Preview.
- [x] Qt Print Support/Widgets runtime validation.
- [x] VC143 runtime bundled side-by-side.
- [x] Desktop shortcut startup smoke test.
- [x] Real PDF-open smoke test.
- [x] Clean-upgrade smoke test removes a simulated obsolete file.

## Android
- [x] Package ID remains `org.orbispdf.app` for update continuity.
- [x] versionName `7.1.0`.
- [x] versionCode `70100`.
- [x] Target API 36 / minimum API 28.
- [x] Signed installable CI APK preserved before AAB build.
- [x] Production signing secrets are excluded from source.

## Before release to users
- [ ] Confirm the new GitHub Actions run is fully green.
- [ ] Install `MaenPDF-Setup.exe` over an older Windows installation and verify clean upgrade behavior on a real PC.
- [ ] Test Print and Print Preview with at least one physical or virtual Windows printer.
- [ ] Install the APK on at least one Android 9/API 28 device and one current Android device.
- [ ] Open/save a representative PDF interoperability corpus.

GitHub Actions is the final native compile/package gate for this source bundle.
