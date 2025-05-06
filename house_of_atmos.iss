
#define MyAppName "House of Atmos"
#define MyAppVersion "0.3.0"
#define MyAppPublisher "schwalbe_t"
#define MyAppURL "https://www.github.com/schwalbe-t/house_of_atmos"
#define MyAppExeName "house_of_atmos.exe"
#define MyAppSrcDir "E:\hoa-v0.3.0"

[Setup]
AppId={{29C4F25C-F217-41C5-999F-B5F9C65C8B8F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={userappdata}\.house_of_atmos
UninstallDisplayIcon={app}\res\icon.ico
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
DisableDirPage=yes
LicenseFile={#MyAppSrcDir}\LICENSE
PrivilegesRequired=lowest
OutputBaseFilename=house_of_atmos_v{#MyAppVersion}
SetupIconFile={#MyAppSrcDir}\res\icon.ico
SolidCompression=yes
WizardStyle=modern
WizardImageFile={#MyAppSrcDir}\res\wizard_banner.bmp
WizardSmallImageFile={#MyAppSrcDir}\res\wizard_icon.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "bulgarian"; MessagesFile: "compiler:Languages\Bulgarian.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#MyAppSrcDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSrcDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSrcDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSrcDir}\res\*"; DestDir: "{app}\res"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\res\icon.ico"
Name: "{userdesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\res\icon.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\settings.json"

