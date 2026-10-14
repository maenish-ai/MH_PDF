# MaenPDF v7.5.1 — Release preparation

- GitHub tag: `v7.5.1` (publish after successful Windows and Android builds).
- Windows installer: `7.5.1`; Android `versionName=7.5.1`, `versionCode=70501`.
- Runtime application version: `7.5.1`.
- Update prompts compare the running version against the latest release. A successfully installed v7.5.1 must not be offered v7.5.1 again.
- Downloading an installer alone does not change the installed version.
- Do not publish the release before signed binaries, checksums, and platform installation tests are complete.
- Windows and Android silent unattended installation is not yet implemented; see UPDATE_SYSTEM.md.
