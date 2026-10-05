from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[1]
required=['.github/workflows/ci.yml','android/AndroidManifest.xml','android/README.md','docs/GITHUB_RELEASE.md','CMakeLists.txt','.gitignore']
checks={f'file:{x}':(root/x).exists() for x in required}
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
wf=(root/'.github/workflows/ci.yml').read_text(encoding='utf-8')
play=(root/'.github/workflows/play-release.yml').read_text(encoding='utf-8')
manifest=(root/'android/AndroidManifest.xml').read_text(encoding='utf-8')
ard=(root/'android/README.md').read_text(encoding='utf-8')
checks.update({
 'v6 semantic version':'project(MaenPDF VERSION 6.0.8' in cm,
 'stable package id':'org.orbispdf.app' in cm and 'org.orbispdf.app' in ard,
 'version code fixed':'QT_ANDROID_VERSION_CODE 60008' in cm,
 'api36 target':'QT_ANDROID_TARGET_SDK_VERSION 36' in cm and "ANDROID_API: '36'" in wf,
 'min api28':'QT_ANDROID_MIN_SDK_VERSION 28' in cm and "ANDROID_MIN_API: '28'" in wf,
 'Qt pinned':'QT_VERSION: \'6.11.2\'' in wf,
 'NDK pinned':"ANDROID_NDK: '27.2.12479018'" in wf,
 'Windows build':'windows-2022' in wf and 'windeployqt' in wf and '--compiler-runtime' in wf,
 'Windows compiler matches Qt MSVC kit':'Visual Studio 17 2022' in wf and '-A x64' in wf and 'build/Release/MaenPDF.exe' in wf,
 'Windows portable startup smoke test':'Smoke test portable MaenPDF executable' in wf and 'QT_QPA_PLATFORM = "offscreen"' in wf,
 'Windows installer built':'MaenPDF-Setup.exe' in wf and 'installer\\MaenPDF.iss' in wf,
 'Windows installed app smoke test':'Smoke test installed MaenPDF' in wf and 'MaenPDF-Smoke' in wf,
 'Windows icon resource':'platform/windows/MaenPDF.rc.in' in cm and (root/'assets/maenpdf.ico').exists(),
 'Android APK/AAB':'--target apk' in wf and '--target aab' in wf,
 'CI produces signed installable Android test APK':'-DQT_ANDROID_SIGN_APK=ON' in wf and 'MaenPDF-Installable-Test.apk' in wf and 'apksigner' in wf,
 'CI Android artifact no longer labeled unsigned':'MaenPDF-Android-Installable-Test' in wf and 'MaenPDF-Android-unsigned' not in wf,
 'CI Android signed APK preserved before AAB rebuild':('Verify and preserve signed test APK' in wf and 'Build AAB for validation' in wf and wf.index('Verify and preserve signed test APK') < wf.index('Build AAB for validation')),
 'CI targets explicit signed APK output':"-name '*-signed.apk'" in wf and 'MaenPDF-Installable-Test.apk' in wf,
 'Android setup avoids obsolete SDK tools package':'packages: platform-tools' in wf and 'packages: platform-tools' in play,
 'Android cross-compile host Qt path':'-DQT_HOST_PATH="$RUNNER_TEMP/Qt/${QT_VERSION}/gcc_64"' in wf and '-DQT_HOST_PATH="$RUNNER_TEMP/Qt/${QT_VERSION}/gcc_64"' in play,
 'Android host Qt validated':'gcc_64/lib/cmake/Qt6/Qt6Config.cmake' in wf and 'gcc_64/lib/cmake/Qt6/Qt6Config.cmake' in play,
 'Android module install has library destination':'if(ANDROID)' in cm and 'LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"' in cm and 'include(GNUInstallDirs)' in cm,
 'Play workflow uses Java 21 action v5':'actions/setup-java@v5' in play and "java-version: '21'" in play,
 'GitHub Actions checkout uses Node24 release':'actions/checkout@v7.0.1' in wf and 'actions/checkout@v7.0.1' in play,
 'GitHub Actions setup-python uses Node24 release':'actions/setup-python@v7.0.0' in wf and 'actions/setup-python@v7.0.0' in play,
 'Android setup uses Node24 release':'android-actions/setup-android@v4.0.4' in wf and 'android-actions/setup-android@v4.0.4' in play,
 'Artifact upload uses Node24 release':'actions/upload-artifact@v7.0.1' in wf and 'actions/upload-artifact@v7.0.1' in play,
 'No deprecated Node20 action refs':all(x not in wf+play for x in ['actions/checkout@v4','actions/setup-python@v5','android-actions/setup-android@v3','actions/upload-artifact@v4']),
 'audit before builds':wf.count('needs: source-audit') >= 2,
 'QtPdf extension requested':'-m qtpdf' in wf and 'aqtinstall.git' in wf,
 'Android manifest Qt lib metadata':'android.app.lib_name' in manifest,
 'Android insertion markers':'%%INSERT_PERMISSIONS' in manifest and '%%INSERT_FEATURES' in manifest,
 'PDF open intent':'android.intent.action.VIEW' in manifest and 'application/pdf' in manifest,
 'no custom gradle override':not (root/'android/build.gradle').exists(),
 'signing secrets excluded':'*.jks' in (root/'.gitignore').read_text(encoding='utf-8'),
 'updates documented':'same package ID' in ard and 'same signing key' in ard,
 'legacy honesty':'older than API 28' in ard,
})
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
