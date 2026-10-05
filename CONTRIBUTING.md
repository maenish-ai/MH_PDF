# Contributing to MaenPDF

Thank you for helping improve MaenPDF.

## Principles

- Keep MaenPDF fast, local-first, bilingual, and usable on modest hardware.
- Do not add document cloud upload, mandatory accounts, paywalls, advertising inside documents, or hidden telemetry.
- Do not claim a PDF capability until it is implemented and covered by a regression check.
- English and Arabic UI strings must be added together.
- Keep security-sensitive parsing and external-tool execution bounded by timeouts and explicit user actions.

## Before opening a pull request

Run:

```bash
python scripts/preflight.py
```

The CI must pass source, localization, security, performance, Windows startup, Windows installer, Android APK, and Android AAB gates.

## Attribution

Contributors may add their name to release notes or the contributor list when their change is accepted. The product remains named **MaenPDF**.
