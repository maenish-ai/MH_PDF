# Orbis PDF 6.0 — Release Checklist

## Before pushing to GitHub
- [x] Package ID fixed: `org.orbispdf.app`
- [x] versionName `6.0.0`
- [x] versionCode `60000`
- [x] English and Arabic catalogs have identical keys
- [x] English catalog contains no Arabic characters
- [x] Arabic catalog has no untranslated Latin UI words (file-extension patterns excluded)
- [x] Arabic layout switches to RTL
- [x] Lazy source-backed page opening and bounded render cache
- [x] Safe save / recovery / diagnostics retained
- [x] Android manifest contains Qt deployment markers and PDF open intent
- [x] CI has audit → Windows → Android build gates
- [x] Play release signing workflow is manual and secrets are not committed

## After first GitHub push
- [ ] Confirm **Orbis PDF CI** is green on the actual GitHub hosted runners.
- [ ] Download and smoke-test Windows artifact.
- [ ] Install APK on at least one API 28-class device/emulator and one current Android device.
- [ ] Open/save a cross-reader PDF corpus and compare output.
- [ ] Configure Play App Signing/upload-key secrets only when the Play Console project exists.

A hosted compile cannot be honestly certified by a source-only environment without Qt; GitHub Actions is the final compilation and packaging gate.
