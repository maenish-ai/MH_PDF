#define MyAppName "MaenPDF"
#define MyAppVersion "7.2.0"
#define MyAppPublisher "MaenPDF"
#define MyAppExeName "MaenPDF.exe"
#define MySettingsGeneration "7100"

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
ChangesAssociations=yes
VersionInfoVersion=7.2.0.0
VersionInfoProductName={#MyAppName}
VersionInfoDescription=MaenPDF Installer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce
Name: "resetsettings"; Description: "Reset MaenPDF preferences and cache"; GroupDescription: "Clean installation:"; Flags: unchecked

; Every update gets a clean application directory so obsolete Qt plugins,
; runtimes or files from an older build can never remain mixed with the new one.
; User PDF documents are never stored here and are never touched.
[InstallDelete]
Type: filesandordirs; Name: "{app}\*"

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\MaenPDF"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\MaenPDF"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch MaenPDF"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\Classes\MaenPDF.Document"; ValueType: string; ValueName: ""; ValueData: "MaenPDF PDF Document"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\MaenPDF.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\MaenPDF.exe,0"
Root: HKCU; Subkey: "Software\Classes\MaenPDF.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\MaenPDF.exe"" ""%1"""
Root: HKCU; Subkey: "Software\Classes\.pdf\OpenWithProgids"; ValueType: none; ValueName: "MaenPDF.Document"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MaenPDF\Installer"; ValueType: string; ValueName: "SettingsGeneration"; ValueData: "{#MySettingsGeneration}"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MaenPDF\Installer"; ValueType: string; ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletevalue

[Code]
function NeedsFirstCleanMigration(): Boolean;
var
  Generation: String;
begin
  Result := True;
  if RegQueryStringValue(HKCU, 'Software\MaenPDF\Installer', 'SettingsGeneration', Generation) then
    Result := CompareText(Generation, '{#MySettingsGeneration}') < 0;
end;

procedure ResetMaenPDFUserState();
begin
  { QSettings on Windows lives under this application registry key. }
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\MaenPDF\MaenPDF');

  { Logs/cache/recovery data are application-owned and safe to recreate. }
  DelTree(ExpandConstant('{localappdata}\MaenPDF'), True, True, True);
  DelTree(ExpandConstant('{userappdata}\MaenPDF'), True, True, True);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    { One-time migration from pre-7.1 installations. Future 7.1+ updates keep
      compatible preferences unless the user explicitly chooses reset. }
    if NeedsFirstCleanMigration() or WizardIsTaskSelected('resetsettings') then
      ResetMaenPDFUserState();
  end;
end;
