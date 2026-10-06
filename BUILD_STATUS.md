# MaenPDF 7.2.0 — Build Status

## Local verification
This bundle is required to pass all source-level release gates before packaging:
- source/reliability audit;
- EN/AR localization parity audit;
- release/packaging audit;
- engine v7 architecture audit;
- security audit;
- performance audit;
- open-source/brand audit;
- interaction-specific static audit;
- JSON, XML and GitHub Actions YAML parsing.

Run:

```bash
python scripts/preflight.py
```

## 7.2 interaction changes
- Single-page `Flickable` replaced by a virtualized continuous `ListView` document surface.
- Real PDF text selection added through Qt PDF selection geometry.
- Selection copy and multi-rectangle text highlighting added.
- Live draw preview and styled ink commit added.
- Crop and redaction now use staged preview / Apply / Cancel interaction.
- Wheel scrolling is explicitly forwarded even when the page interaction layer owns the pointer.
- Page renders/thumbnails are asynchronous; Qt PDF backend calls are mutex-serialized.
- Long local operations pump non-input events to avoid Windows `Not Responding` during legitimate work.
- CI uses a three-page fixture and a dedicated interaction smoke mode before installer generation.

## Native build authority
This working environment does not include the complete Qt 6.11 Windows and Android kits. GitHub Actions remains the authoritative native compile, package and runtime gate after the source bundle is uploaded.

## Android
The standard CI workflow continues to produce `MaenPDF-Installable-Test.apk` with an ephemeral CI-only signing key. The AAB remains a validation artifact; production Play signing stays isolated in `.github/workflows/play-release.yml`.
