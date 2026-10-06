from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
required=['.github/workflows/ci.yml','android/AndroidManifest.xml','android/README.md','docs/GITHUB_RELEASE.md','CMakeLists.txt','.gitignore','LICENSE','SECURITY.md','BRAND_POLICY.md']
checks={f'file:{x}':(root/x).exists() for x in required}
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
wf=(root/'.github/workflows/ci.yml').read_text(encoding='utf-8')
play=(root/'.github/workflows/play-release.yml').read_text(encoding='utf-8')
manifest=(root/'android/AndroidManifest.xml').read_text(encoding='utf-8')
ard=(root/'android/README.md').read_text(encoding='utf-8')
iss=(root/'installer/MaenPDF.iss').read_text(encoding='utf-8')
pre=(root/'scripts/preflight.py').read_text(encoding='utf-8')
checks.update({
 'v7.2 semantic version':'project(MaenPDF VERSION 7.2.1' in cm,
 'v7 QML module':'VERSION 7.0' in cm,
 'stable package id':'org.orbispdf.app' in cm and 'org.orbispdf.app' in ard,
 'version code 70201':'QT_ANDROID_VERSION_CODE 70201' in cm,
 'version name 7.2':'QT_ANDROID_VERSION_NAME "7.2.1"' in cm,
 'api36 target':'QT_ANDROID_TARGET_SDK_VERSION 36' in cm and "ANDROID_API: '36'" in wf,
 'min api28':'QT_ANDROID_MIN_SDK_VERSION 28' in cm and "ANDROID_MIN_API: '28'" in wf,
 'Qt pinned':'QT_VERSION: \'6.11.2\'' in wf,
 'NDK pinned':"ANDROID_NDK: '27.2.12479018'" in wf,
 'Windows build':'windows-2022' in wf and 'scripts/deploy_windows.ps1' in wf,
 'Windows compiler matches Qt MSVC kit':'Visual Studio 17 2022' in wf and '-A x64' in wf and 'build/Release/MaenPDF.exe' in wf,
 'Windows native portable startup smoke test':'Smoke test portable MaenPDF with native Windows platform' in wf and 'Remove-Item Env:QT_QPA_PLATFORM' in wf,
 'Windows real PDF open smoke test':'Smoke test portable MaenPDF opening PDF fixture' in wf and 'create_smoke_pdf.py' in wf and 'Cannot assign to non-existent property' in wf,
 'Windows interaction engine smoke test':'Exercise text selection drawing crop and redaction engine' in wf and '--interaction-smoke' in wf and 'INTERACTION_SMOKE_PASS' in wf,
 'Windows installer v7.2':'#define MyAppVersion "7.2.1"' in iss and 'VersionInfoVersion=7.2.1.0' in iss,
 'Windows clean application upgrade':'[InstallDelete]' in iss and 'Type: filesandordirs; Name: "{app}\\*"' in iss,
 'Windows one-time settings migration':'SettingsGeneration' in iss and 'ResetMaenPDFUserState' in iss and 'NeedsFirstCleanMigration' in iss,
 'desktop native print support':'Qt6::PrintSupport' in cm and 'core/PrintService.cpp' in cm,
 'Windows installer built':'MaenPDF-Setup.exe' in wf and 'installer\\MaenPDF.iss' in wf,
 'Windows installed shortcut smoke test':'Smoke test installed MaenPDF from desktop shortcut' in wf and 'MaenPDF.lnk' in wf and 'WScript.Shell' in wf,
 'Windows icon resource':'platform/windows/MaenPDF.rc.in' in cm and (root/'assets/maenpdf.ico').exists(),
 'Windows standalone MSVC runtime gate':'Microsoft.VC143.CRT' in (root/'scripts/deploy_windows.ps1').read_text(encoding='utf-8') and 'vswhere.exe' in (root/'scripts/deploy_windows.ps1').read_text(encoding='utf-8'),
 'Android APK/AAB':'--target apk' in wf and '--target aab' in wf,
 'CI produces signed installable Android test APK':'-DQT_ANDROID_SIGN_APK=ON' in wf and 'MaenPDF-Installable-Test.apk' in wf and 'apksigner' in wf,
 'CI signed APK preserved before AAB':('Verify and preserve signed test APK' in wf and 'Build AAB for validation' in wf and wf.index('Verify and preserve signed test APK') < wf.index('Build AAB for validation')),
 'Android setup current':'android-actions/setup-android@v4.0.4' in wf and 'android-actions/setup-android@v4.0.4' in play,
 'Android cross-compile host Qt path':'-DQT_HOST_PATH="$RUNNER_TEMP/Qt/${QT_VERSION}/gcc_64"' in wf and '-DQT_HOST_PATH="$RUNNER_TEMP/Qt/${QT_VERSION}/gcc_64"' in play,
 'Android module install has library destination':'LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"' in cm and 'include(GNUInstallDirs)' in cm,
 'Play workflow Java 21':'actions/setup-java@v5' in play and "java-version: '21'" in play,
 'GitHub checkout Node24 release':'actions/checkout@v7.0.1' in wf and 'actions/checkout@v7.0.1' in play,
 'GitHub setup-python Node24 release':'actions/setup-python@v7.0.0' in wf and 'actions/setup-python@v7.0.0' in play,
 'Artifact upload Node24 release':'actions/upload-artifact@v7.0.1' in wf and 'actions/upload-artifact@v7.0.1' in play,
 'No deprecated Node20 refs':all(x not in wf+play for x in ['actions/checkout@v4','actions/setup-python@v5','android-actions/setup-android@v3','actions/upload-artifact@v4']),
 'audit before builds':wf.count('needs: source-audit') >= 2,
 'QtPdf extension requested':'-m qtpdf' in wf and 'aqtinstall.git' in wf,
 'Android manifest Qt lib metadata':'android.app.lib_name' in manifest,
 'Android insertion markers':'%%INSERT_PERMISSIONS' in manifest and '%%INSERT_FEATURES' in manifest,
 'PDF open intent':'android.intent.action.VIEW' in manifest and 'application/pdf' in manifest,
 'signing secrets excluded':'*.jks' in (root/'.gitignore').read_text(encoding='utf-8'),
 'updates documented':'same package ID' in ard and 'same signing key' in ard,
 'v7 audits in preflight':all(x in pre for x in ['engine_v7_audit.py','security_audit.py','performance_audit.py','interaction_audit.py','open_source_audit.py']),
 'v7 audits in CI':all(x in wf for x in ['engine_v7_audit.py','security_audit.py','performance_audit.py','interaction_audit.py','open_source_audit.py']),
})
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
