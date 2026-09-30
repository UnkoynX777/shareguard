#define AppName "ShareGuard"
#define AppVersion "0.3.1"
#define AppPublisher "UnkoynX777"
#define HostName "com.shareguard.native"
#define ExtensionId "bdkcdhphggeglifemnakdlcfbhcoempk"
#define FirefoxId "shareguard@shareguard.local"
#define InstallGuide "https://github.com/UnkoynX777/shareguard/blob/main/docs/INSTALLATION.md"

[Setup]
AppId={{A7E3C1D4-6B58-4F0E-9C2A-1D5E8F0A3B71}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL=https://github.com/UnkoynX777/shareguard
AppSupportURL=https://github.com/UnkoynX777/shareguard/issues
AppUpdatesURL=https://github.com/UnkoynX777/shareguard/releases/latest
DefaultDirName={localappdata}\ShareGuard
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
OutputBaseFilename=ShareGuard-Setup-v{#AppVersion}-x64
Compression=lzma2
SolidCompression=yes
UninstallDisplayName={#AppName}
WizardStyle=modern

[Files]
Source: "..\native\build\Release\shareguard-native.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\extension\dist\chromium\*"; DestDir: "{app}\Extension\Chromium"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.map,*.ts,*.tsx"

[Registry]
Root: HKCU; Subkey: "Software\Google\Chrome\NativeMessagingHosts\{#HostName}"; ValueType: string; ValueName: ""; ValueData: "{app}\{#HostName}.chromium.json"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Microsoft\Edge\NativeMessagingHosts\{#HostName}"; ValueType: string; ValueName: ""; ValueData: "{app}\{#HostName}.chromium.json"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Mozilla\NativeMessagingHosts\{#HostName}"; ValueType: string; ValueName: ""; ValueData: "{app}\{#HostName}.firefox.json"; Flags: uninsdeletekey

[Icons]
Name: "{group}\{#AppName}"; Filename: "{win}\explorer.exe"; Parameters: "{#InstallGuide}"
Name: "{group}\Install Browser Extension"; Filename: "{win}\explorer.exe"; Parameters: """{app}\Extension\Chromium"""
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{#InstallGuide}"; Description: "Open the installation guide"; Flags: postinstall shellexec nowait skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\{#HostName}.json"
Type: files; Name: "{app}\{#HostName}.chromium.json"
Type: files; Name: "{app}\{#HostName}.firefox.json"
Type: filesandordirs; Name: "{app}\Extension"

[Code]
function BrowserFile(const RelativePath: String): Boolean;
begin
  Result := FileExists(ExpandConstant('{pf}\') + RelativePath) or FileExists(ExpandConstant('{pf32}\') + RelativePath);
end;

procedure WriteHostManifest(const FileName, Body: String);
begin
  SaveStringToFile(ExpandConstant('{app}\') + FileName, Body, False);
end;

function EscapeJsonPath(const Value: String): String;
var
  Index: Integer;
begin
  Result := '';
  for Index := 1 to Length(Value) do
  begin
    if Value[Index] = '\' then
      Result := Result + '\\'
    else
      Result := Result + Value[Index];
  end;
end;

procedure WriteNativeManifests();
var
  ExePath: String;
  Common: String;
  ChromiumJson: String;
  FirefoxJson: String;
begin
  ExePath := EscapeJsonPath(ExpandConstant('{app}\shareguard-native.exe'));
  Common :=
    '{' + #13#10 +
    '  "name": "{#HostName}",' + #13#10 +
    '  "description": "ShareGuard native audio helper",' + #13#10 +
    '  "path": "' + ExePath + '",' + #13#10 +
    '  "type": "stdio",' + #13#10;
  ChromiumJson :=
    Common +
    '  "allowed_origins": [' + #13#10 +
    '    "chrome-extension://{#ExtensionId}/"' + #13#10 +
    '  ]' + #13#10 +
    '}' + #13#10;
  FirefoxJson :=
    Common +
    '  "allowed_extensions": [' + #13#10 +
    '    "{#FirefoxId}"' + #13#10 +
    '  ]' + #13#10 +
    '}' + #13#10;
  WriteHostManifest('{#HostName}.chromium.json', ChromiumJson);
  WriteHostManifest('{#HostName}.firefox.json', FirefoxJson);
  if BrowserFile('Google\Chrome\Application\chrome.exe') then
    Log('Google Chrome detected');
  if BrowserFile('Microsoft\Edge\Application\msedge.exe') then
    Log('Microsoft Edge detected');
  if BrowserFile('Mozilla Firefox\firefox.exe') then
    Log('Mozilla Firefox detected');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    WriteNativeManifests();
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpFinished then
  begin
    WizardForm.FinishedHeadingLabel.Caption := 'ShareGuard was installed successfully.';
    WizardForm.FinishedLabel.Caption :=
      'One final step: add the ShareGuard extension to your browser.' + #13#10 + #13#10 +
      'Chrome and Edge load this folder with Developer mode and Load unpacked:' + #13#10 +
      ExpandConstant('{app}\Extension\Chromium') + #13#10 + #13#10 +
      'Firefox uses the signed extension from the GitHub release. The installation guide explains both.';
  end;
end;
