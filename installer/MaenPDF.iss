#define MyAppName "MaenPDF"
#define MyAppVersion "6.0.11"
#define MyAppPublisher "MaenPDF"
#define MyAppExeName "MaenPDF.exe"

[Setup]
AppId={{77D8DA72-BF81-4D2A-8DB1-771C4040586E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\MaenPDF
DefaultGroupName=MaenPDF
DisableProgramGroupPage=yes
OutputDir=..\installer-output
OutputBaseFilename=MaenPDF-Setup
SetupIconFile=..\assets\maenpdf.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
RestartApplications=no
VersionInfoVersion=6.0.11.0
VersionInfoProductName={#MyAppName}
VersionInfoDescription=MaenPDF Installer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\MaenPDF"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\MaenPDF"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch MaenPDF"; Flags: nowait postinstall skipifsilent
