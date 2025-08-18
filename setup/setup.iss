#define MyAppName "ClamAV Antivirus"
#define MyAppVersion "1.4.3-r1"
#define MyAppPublisher "Gianluigi Tiesi <sherpya@gmail.com>"
#define MyAppURL "https://oss.netfarm.it/clamav/"

#define MinVersion "10.0"
#define DistDir "..\..\dist\clamav-" + MyAppVersion + "-x64"

[Setup]
AppId={{DFE4DC51-C7CA-4004-9740-A982518A616C}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=none
MinVersion={#MinVersion}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={sd}\ClamAV
DefaultGroupName={#MyAppName}
LicenseFile=README.rtf
OutputBaseFilename=clamav-{#MyAppVersion}-setup
SetupIconFile=..\resources\clamav.ico
SolidCompression=yes
WizardStyle=modern
;PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "main"; Description: "Main Files"; Types: full compact custom; Flags: fixed
Name: "clamd"; Description: "Clamd Files"; Types: full compact custom; Flags: fixed
Name: "extra"; Description: "Extra Files"; Types: full custom; Flags: fixed

[Files]
; libclamav
Source: "{#DistDir}\libclamav.dll"; DestDir: "{app}"; Components: main
Source: "{#DistDir}\libclamunrar_iface.dll"; DestDir: "{app}"; Components: main
Source: "{#DistDir}\libclamunrar.dll"; DestDir: "{app}"; Components: main
; clamscan
Source: "{#DistDir}\clamscan.exe"; DestDir: "{app}"; Components: main
; freshclam
Source: "{#DistDir}\libfreshclam.dll"; DestDir: "{app}"; Components: main
Source: "{#DistDir}\freshclam.exe"; DestDir: "{app}"; Components: main
; clamd/clamdscan
Source: "{#DistDir}\clamd.exe"; DestDir: "{app}"; Components: clamd
Source: "{#DistDir}\clamdscan.exe"; DestDir: "{app}"; Components: clamd
; sigtool
Source: "{#DistDir}\sigtool.exe"; DestDir: "{app}"; Components: extra
; clambc
Source: "{#DistDir}\clambc.exe"; DestDir: "{app}"; Components: extra
; clamdtop
Source: "{#DistDir}\clamdtop.exe"; DestDir: "{app}"; Components: extra
; copyright files
Source: "{#DistDir}\copyright\*";  DestDir: "{app}/copyright"; Components: main; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
Name: {app}\db; Permissions: authusers-full

[Registry]
Root: HKLM; Subkey: "SOFTWARE\ClamAV"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "SOFTWARE\ClamAV"; ValueType: string; ValueName: "ConfDir"; ValueData: "{app}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\ClamAV"; ValueType: string; ValueName: "DataDir"; ValueData: "{app}\db"; Flags: uninsdeletevalue

[UninstallDelete]
Type: files; Name: "{app}\freshclam.conf"; Components: main
Type: files; Name: "{app}\clamd.conf"; Components: clamd

[Code]
procedure InitializeWizard();
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;
end;

procedure CreateFreshclamConf;
var
  sFile, sContent: string;
begin
  sFile := ExpandConstant('{app}\freshclam.conf');
  if not FileExists(sFile) then
  begin
    sContent := 'DatabaseMirror database.clamav.net' + #13#10 +
                'DNSDatabaseInfo current.cvd.clamav.net' + #13#10
    if not SaveStringToFile(sFile, sContent, False) then
    begin
      MsgBox('Error creating config file: ' + sFile, mbError, MB_OK);
    end;
  end;
end;

procedure CreateClamdConf;
var
  sFile, sContent: string;
begin
  sFile := ExpandConstant('{app}\clamd.conf');
  if not FileExists(sFile) then
  begin
    sContent := 'TCPSocket 3310' + #13#10 +
                'TCPAddr 127.0.0.1' + #13#10 +
                'MaxThreads 2' + #13#10 +
                'LogFile ' + ExpandConstant('{app}\clamd.log') + #13#10 +
                'DatabaseDirectory ' + ExpandConstant('{app}\db') + #13#10;
    if not SaveStringToFile(sFile, sContent, False) then
    begin
      MsgBox('Error creating config file: ' + sFile, mbError, MB_OK);
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
      CreateFreshclamConf;
      if WizardIsComponentSelected('clamd') then
        CreateClamdConf;
  end;
end;
