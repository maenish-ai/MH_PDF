# MaenPDF Update System — Windows + Android

## Loop prevention

The update manager compares the **running application's version** (`QCoreApplication::applicationVersion()`) against the latest published GitHub Release tag using `QVersionNumber`. A release is offered only if it is newer. Automatic notifications are deduplicated with a persistent `QSettings` key scoped to the **installed-version -> release-version** pair. A manually requested check still shows the current result. Merely downloading a package **does not** change the installed version.

**Release requirement:** Each actual new build must update `QGuiApplication::setApplicationVersion(...)` in `app/main.cpp` to match its GitHub release tag. Otherwise the app will keep identifying itself as the old version. For Android, also increment `versionCode` and set `versionName`; signing keys must remain consistent.

## Deployment

The manager discovers GitHub Releases, chooses an OS-specific asset, downloads it, and verifies its GitHub-provided SHA-256 digest. It does **not** yet provide fully unattended installation: Android APK installation generally needs an OS confirmation; Windows installers may need elevation and should be launched by a separate trusted updater after the application closes. Do not advertise silent install until this is implemented and tested.

## Release checklist

1. Bump the runtime version, release tag, and package metadata consistently.
2. Build signed Windows installer and signed Android package.
3. Publish both as GitHub Release assets with SHA-256 digests.
4. Test 7.5.0 -> 7.5.1: one automatic prompt only, including after restart.
5. Test successful install: application reports 7.5.1 and no longer offers 7.5.1.
6. Test failed install: installed version remains unchanged; manual check still offers update.
7. Test newer 7.5.2: prompt appears once for this new target.

**Security note:** A SHA-256 digest returned from the same GitHub release API is an integrity check, not independent cryptographic publisher authentication. Signed packages and verified publishing provenance are required for a secure unattended update channel.
