; ClamWin Free Antivirus — Inno Setup script
; Packages the native C++ GUI build (clamwin-gui-cpp).
;
; Build outputs are taken from {#BuildDirWin9x}, {#BuildDir32}, {#BuildDir64}.
; Override individual dirs at compile time, e.g.:
;   iscc /DBuildDir64=C:\path\to\x64-build Setup.iss

#DEFINE IncludeCVD
#DEFINE AppVersion "1.5.1"
#DEFINE BuildDirWin9x "..\..\..\build-x86-mingw-win98"
#DEFINE BuildDir32    "..\..\..\build-x86-mingw-winxp"
#DEFINE BuildDir64    "..\..\..\build-x64"

[Setup]
AppName=ClamWin Free Antivirus
AppVerName=ClamWin Free Antivirus {#AppVersion}
AppVersion={#AppVersion}
AppPublisher=ClamWin Pty Ltd
AppPublisherURL=http://www.clamwin.com/
AppSupportURL=http://www.clamwin.com/
AppUpdatesURL=http://www.clamwin.com/
DefaultDirName={code:BaseDir}\ClamWin
DefaultGroupName=ClamWin Antivirus
LicenseFile=Setupfiles\License.rtf
AllowNoIcons=true
MinVersion=4.1.1998,5.0.2195
ShowLanguageDialog=no
LanguageDetectionMethod=none
OutputDir=Output
OutputBaseFilename=clamwin-{#AppVersion}-setup
Compression=lzma/ultra
InternalCompressLevel=max
SolidCompression=true
WizardImageFile=Setupfiles\WizModernImage.bmp
WizardSmallImageFile=Setupfiles\WizModernSmallImage.bmp

[Components]
Name: ClamAV;       Description: ClamAV Engine Files;              Flags: fixed; Types: full custom typical
Name: ClamWin;      Description: ClamWin GUI;                      Flags: fixed; Types: full custom typical
Name: ExplorerShell; Description: Windows Explorer Integration;    Types: full custom typical

[Types]
Name: typical; Description: Typical Installation
Name: custom;  Description: Custom Installation; Flags: iscustom
Name: full;    Description: Full Installation

[Tasks]
Name: DownloadDB;  Description: Download Virus Database Files after install; GroupDescription: Download; Components: ClamAV
Name: desktopicon; Description: Create a &desktop icon; GroupDescription: Additional icons:; Flags: unchecked

[Files]
; ── ClamWin GUI ───────────────────────────────────────────────────────────────
; clamwin.exe
Source: {#BuildDirWin9x}\clamwin.exe; DestDir: {app}\bin; Components: ClamWin; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\clamwin.exe;    DestDir: {app}\bin; Components: ClamWin; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\clamwin.exe;    DestDir: {app}\bin; Components: ClamWin; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; libExpShell.dll
Source: {#BuildDirWin9x}\libExpShell.dll; DestDir: {app}\bin; Components: ExplorerShell; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\libExpShell.dll;    DestDir: {app}\bin; Components: ExplorerShell; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\libExpShell.dll;    DestDir: {app}\bin; Components: ExplorerShell; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; ── ClamAV Engine ─────────────────────────────────────────────────────────────
; clamscan.exe
Source: {#BuildDirWin9x}\clamscan.exe; DestDir: {app}\bin; Components: ClamAV; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\clamscan.exe;    DestDir: {app}\bin; Components: ClamAV; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\clamscan.exe;    DestDir: {app}\bin; Components: ClamAV; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; freshclam.exe
Source: {#BuildDirWin9x}\freshclam.exe; DestDir: {app}\bin; Components: ClamAV; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\freshclam.exe;    DestDir: {app}\bin; Components: ClamAV; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\freshclam.exe;    DestDir: {app}\bin; Components: ClamAV; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; sigtool.exe
Source: {#BuildDirWin9x}\sigtool.exe; DestDir: {app}\bin; Components: ClamAV; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\sigtool.exe;    DestDir: {app}\bin; Components: ClamAV; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\sigtool.exe;    DestDir: {app}\bin; Components: ClamAV; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; libclamav.dll
Source: {#BuildDirWin9x}\libclamav.dll; DestDir: {app}\bin; Components: ClamAV; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\libclamav.dll;    DestDir: {app}\bin; Components: ClamAV; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\libclamav.dll;    DestDir: {app}\bin; Components: ClamAV; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; libfreshclam.dll
Source: {#BuildDirWin9x}\libfreshclam.dll; DestDir: {app}\bin; Components: ClamAV; Check: IsWin9xTier; Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir32}\libfreshclam.dll;    DestDir: {app}\bin; Components: ClamAV; Check: Is32bitNT;   Flags: restartreplace uninsrestartdelete replacesameversion
Source: {#BuildDir64}\libfreshclam.dll;    DestDir: {app}\bin; Components: ClamAV; Check: IsWin64;     Flags: restartreplace uninsrestartdelete replacesameversion

; ── Virus Databases (optional) ────────────────────────────────────────────────
#IFDEF IncludeCVD
Source: cvd\main.cvd;     DestDir: {code:CommonProfileDir}\.clamwin\db; Components: ClamWin; Flags: ignoreversion comparetimestamp; Check: NoMainCld
Source: cvd\daily.cvd;    DestDir: {code:CommonProfileDir}\.clamwin\db; Components: ClamWin; Flags: ignoreversion comparetimestamp; Check: NoDailyCld
Source: cvd\bytecode.cvd; DestDir: {code:CommonProfileDir}\.clamwin\db; Components: ClamWin; Flags: ignoreversion comparetimestamp; Check: NoBytecodeCld
#ENDIF

[Dirs]
Name: {code:CommonProfileDir}\.clamwin\db;           Components: ClamAV; Permissions: authusers-full; Check: IsAllUsers
Name: {code:CommonProfileDir}\.clamwin\log;          Components: ClamAV; Permissions: authusers-full; Check: IsAllUsers
Name: {code:CommonProfileDir}\.clamwin\quarantine;   Components: ClamAV; Permissions: authusers-full; Check: IsAllUsers
Name: {code:CommonProfileDir}\.clamwin\db;           Components: ClamAV; Check: not IsAllUsers
Name: {code:CommonProfileDir}\.clamwin\log;          Components: ClamAV; Check: not IsAllUsers
Name: {code:CommonProfileDir}\.clamwin\quarantine;   Components: ClamAV; Check: not IsAllUsers
Name: {app}\bin; Components: ClamWin ExplorerShell

; ── Remove legacy wxPython files on upgrade ───────────────────────────────────
[InstallDelete]
; Old GUI Python executables
Type: files; Name: {app}\bin\ClamTray.exe
Type: files; Name: {app}\bin\WClose.exe
Type: files; Name: {app}\bin\OlAddin.exe
Type: files; Name: {app}\bin\w9xpopen.exe

; Old Python runtime DLL
Type: files; Name: {app}\bin\python23.dll
Type: files; Name: {app}\bin\unicows.dll

; Old shell extension DLLs (were ExpShell.dll / ExpShell64.dll in wxPython era)
Type: files; Name: {app}\bin\ExpShell.dll
Type: files; Name: {app}\bin\ExpShell64.dll

; Old docs
Type: files; Name: {app}\bin\manual.chm
Type: files; Name: {app}\bin\manual_en.pdf
Type: files; Name: {app}\bin\manual_fr.pdf
Type: files; Name: {app}\bin\manual_nl.pdf
Type: files; Name: {app}\bin\manual_nl.chm

; Entire old wxPython lib and img directories
Type: filesandordirs; Name: {app}\lib
Type: filesandordirs; Name: {app}\bin\img
Type: filesandordirs; Name: {app}\bin\Microsoft.VC80.CRT

; Old CVD format (upgrade to new if present)
Type: files; Name: {code:CommonProfileDir}\.clamwin\db\main.cvd;     Check: IsDeleteCvd
Type: files; Name: {code:CommonProfileDir}\.clamwin\db\daily.cvd;    Check: IsDeleteCvd
Type: files; Name: {code:CommonProfileDir}\.clamwin\db\bytecode.cvd; Check: IsDeleteCvd

[UninstallDelete]
Type: filesandordirs; Name: {app}
Type: filesandordirs; Name: {userappdata}\.clamwin
Type: filesandordirs; Name: {code:CommonProfileDir}\.clamwin

[Icons]
Name: {group}\ClamWin Antivirus;        Filename: {app}\bin\clamwin.exe; WorkingDir: {app}\bin; Comment: ClamWin Free Antivirus; Components: ClamWin
Name: {code:DesktopDir}\ClamWin Antivirus; Filename: {app}\bin\clamwin.exe; WorkingDir: {app}\bin; Comment: ClamWin Free Antivirus; Components: ClamWin; Tasks: desktopicon
Name: {group}\Uninstall ClamWin Free Antivirus; Filename: {uninstallexe}

[Run]
; Download databases after install if user selected the task
Filename: {app}\bin\freshclam.exe; Parameters: "--config-file=""{code:FreshclamConfPath}"""; WorkingDir: {app}\bin; StatusMsg: Downloading Virus Database Files...; Components: ClamAV; Tasks: DownloadDB; Flags: runhidden
; Launch clamwin.exe when done
Filename: {app}\bin\clamwin.exe; WorkingDir: {app}\bin; Flags: nowait postinstall skipifsilent; Description: Launch ClamWin Free Antivirus; Components: ClamWin

[INI]
; Only write these if the key is currently empty — preserves existing config on upgrade
Filename: {app}\bin\ClamWin.conf; Section: ClamAV;   Key: clamscan;       String: {app}\bin\clamscan.exe;    Check: IsIniValueEmpty(ExpandConstant('ClamAV*clamscan*{app}\bin\ClamWin.conf'))
Filename: {app}\bin\ClamWin.conf; Section: ClamAV;   Key: freshclam;      String: {app}\bin\freshclam.exe;   Check: IsIniValueEmpty(ExpandConstant('ClamAV*freshclam*{app}\bin\ClamWin.conf'))
Filename: {app}\bin\ClamWin.conf; Section: ClamAV;   Key: database;       String: {code:CommonProfileDir}\.clamwin\db;          Check: IsIniValueEmpty(ExpandConstant('ClamAV*database*{app}\bin\ClamWin.conf'))
Filename: {app}\bin\ClamWin.conf; Section: ClamAV;   Key: quarantinedir;  String: {code:CommonProfileDir}\.clamwin\quarantine;   Check: IsIniValueEmpty(ExpandConstant('ClamAV*quarantinedir*{app}\bin\ClamWin.conf'))
Filename: {app}\bin\ClamWin.conf; Section: ClamAV;   Key: logfile;        String: {code:CommonProfileDir}\.clamwin\log\ClamScanLog.txt;   Check: IsIniValueEmpty(ExpandConstant('ClamAV*logfile*{app}\bin\ClamWin.conf'))
Filename: {app}\bin\ClamWin.conf; Section: Updates;  Key: dbupdatelogfile; String: {code:CommonProfileDir}\.clamwin\log\ClamUpdateLog.txt; Check: IsIniValueEmpty(ExpandConstant('Updates*dbupdatelogfile*{app}\bin\ClamWin.conf'))
Filename: {app}\bin\ClamWin.conf; Section: Updates;  Key: time;            String: {code:CurTime};            Check: IsIniValueEmpty(ExpandConstant('Updates*time*{app}\bin\ClamWin.conf'))

[Registry]
; ── ClamWin path ────────────────────────────────────────────────────────────────
Root: HKLM;   Subkey: Software\ClamWin; ValueType: string; ValueName: Path;    ValueData: {app}\bin; Flags: uninsdeletekey deletevalue; Components: ClamWin; Check: IsAllUsers
Root: HKLM;   Subkey: Software\ClamWin; ValueType: dword;  ValueName: Version; ValueData: 10501;     Flags: uninsdeletekey deletevalue; Components: ClamWin; Check: IsAllUsers
Root: HKLM64; Subkey: Software\ClamWin; ValueType: string; ValueName: Path;    ValueData: {app}\bin; Flags: uninsdeletekey deletevalue; Components: ClamWin; Check: IsAllUsers and IsWin64
Root: HKCU;   Subkey: Software\ClamWin; ValueType: string; ValueName: Path;    ValueData: {app}\bin; Flags: uninsdeletekey deletevalue; Components: ClamWin; Check: not IsAllUsers
Root: HKCU;   Subkey: Software\ClamWin; ValueType: dword;  ValueName: Version; ValueData: 10501;     Flags: uninsdeletekey deletevalue; Components: ClamWin; Check: not IsAllUsers
Root: HKCU64; Subkey: Software\ClamWin; ValueType: string; ValueName: Path;    ValueData: {app}\bin; Flags: uninsdeletekey deletevalue; Components: ClamWin; Check: not IsAllUsers and IsWin64

; ── Auto-start tray (not needed for new C++ build — clamwin.exe manages itself) ─

; ── Explorer shell extension (all users) ────────────────────────────────────────
Root: HKCR;   Subkey: CLSID\{{65713842-C410-4f44-8383-BFE01A398C90}}\InProcServer32; ValueType: string; ValueData: {app}\bin\libExpShell.dll; Flags: uninsdeletekey; Components: ExplorerShell; Check: IsAllUsers
Root: HKCR;   Subkey: CLSID\{{65713842-C410-4f44-8383-BFE01A398C90}}\InProcServer32; ValueType: string; ValueName: ThreadingModel; ValueData: Apartment; Flags: uninsdeletekey; Components: ExplorerShell; Check: IsAllUsers
Root: HKCR;   Subkey: *\shellex\ContextMenuHandlers\ClamWin; ValueType: string; ValueData: {{65713842-C410-4f44-8383-BFE01A398C90}}; Flags: uninsdeletekey; Components: ExplorerShell; Check: IsAllUsers
Root: HKCR;   Subkey: Folder\shellex\ContextMenuHandlers\ClamWin; ValueType: string; ValueData: {{65713842-C410-4f44-8383-BFE01A398C90}}; Flags: uninsdeletekey; Components: ExplorerShell; Check: IsAllUsers

; ── Explorer shell extension (per-user) ─────────────────────────────────────────
Root: HKCU;   Subkey: Software\Classes\CLSID\{{65713842-C410-4f44-8383-BFE01A398C90}}\InProcServer32; ValueType: string; ValueData: {app}\bin\libExpShell.dll; Flags: uninsdeletekey; Components: ExplorerShell; Check: not IsAllUsers
Root: HKCU;   Subkey: Software\Classes\CLSID\{{65713842-C410-4f44-8383-BFE01A398C90}}\InProcServer32; ValueType: string; ValueName: ThreadingModel; ValueData: Apartment; Flags: uninsdeletekey; Components: ExplorerShell; Check: not IsAllUsers
Root: HKCU;   Subkey: Software\Classes\*\shellex\ContextMenuHandlers\ClamWin; ValueType: string; ValueData: {{65713842-C410-4f44-8383-BFE01A398C90}}; Flags: uninsdeletekey; Components: ExplorerShell; Check: not IsAllUsers
Root: HKCU;   Subkey: Software\Classes\Folder\shellex\ContextMenuHandlers\ClamWin; ValueType: string; ValueData: {{65713842-C410-4f44-8383-BFE01A398C90}}; Flags: uninsdeletekey; Components: ExplorerShell; Check: not IsAllUsers

[Code]

var
  AllUsers:       Boolean;
  SetupCompleted: Boolean;
  Time:           String;
  PreviousVersion, ThisVersion: Integer;
  AllUsersPage:   TInputOptionWizardPage;

//────────────────────────────────────────────────────────────────────────────
// Helpers
//────────────────────────────────────────────────────────────────────────────

function IsWin9xTier(): Boolean;
var Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  Result := not Version.NTPlatform;
end;

function Is32bitNT(): Boolean;
begin
  Result := not IsWin9xTier() and not IsWin64;
end;

function IsAllUsers(): Boolean;
begin
  Result := AllUsers;
end;

function BaseDir(Default: String): String;
begin
  if IsAllUsers() then
    Result := ExpandConstant('{pf}')
  else
    Result := ExpandConstant('{userappdata}');
end;

function CommonProfileDir(Default: String): String;
begin
  if IsAllUsers() then
    Result := RemoveBackslash(ExpandConstant('{commonappdata}'))
  else
    Result := ExpandConstant('{userappdata}');
end;

function DesktopDir(Default: String): String;
begin
  if IsAllUsers() then
    Result := ExpandConstant('{commondesktop}')
  else
    Result := ExpandConstant('{userdesktop}');
end;

function CurTime(Default: String): String;
begin
  Result := Time;
end;

//────────────────────────────────────────────────────────────────────────────
// INI helper — used in [INI] Check: expressions
//────────────────────────────────────────────────────────────────────────────
function IsIniValueEmpty(Param: String): Boolean;
var
  Section, Key, Filename, Val, TempStr: String;
  EndPos: Integer;
begin
  TempStr := Param;

  EndPos  := Pos('*', TempStr);
  Section := Copy(TempStr, 0, EndPos - 1);
  TempStr := Copy(TempStr, EndPos + 1, Length(TempStr) - EndPos);

  EndPos := Pos('*', TempStr);
  Key    := Copy(TempStr, 0, EndPos - 1);
  TempStr := Copy(TempStr, EndPos + 1, Length(TempStr) - EndPos);

  Filename := TempStr;
  Val      := Trim(GetIniString(Section, Key, '', Filename));
  Result   := (Val = '');
end;

//────────────────────────────────────────────────────────────────────────────
// CVD helpers — used in [Files] Check: and [InstallDelete] Check:
//────────────────────────────────────────────────────────────────────────────
function NoMainCld(): Boolean;
var Temp: String;
begin
  Result := not FileExists(CommonProfileDir(Temp) + '\.clamwin\db\main.cld');
end;

function NoDailyCld(): Boolean;
var Temp: String;
begin
  Result := not FileExists(CommonProfileDir(Temp) + '\.clamwin\db\daily.cld');
end;

function NoBytecodeCld(): Boolean;
var Temp: String;
begin
  Result := not FileExists(CommonProfileDir(Temp) + '\.clamwin\db\bytecode.cld');
end;

// Delete old CVD only when upgrading from a version that used a different format
function IsDeleteCvd(): Boolean;
begin
  Result := (PreviousVersion > 0) and (PreviousVersion < 10501);
end;

//────────────────────────────────────────────────────────────────────────────
// Close the old ClamTray if it is running
//────────────────────────────────────────────────────────────────────────────
procedure CloseClamTray();
var
  path, keyname: String;
  resultcode: Integer;
begin
  keyname := 'SOFTWARE\ClamWin';
  path    := '';
  if not RegQueryStringValue(HKEY_CURRENT_USER, keyname, 'Path', path) then
    RegQueryStringValue(HKEY_LOCAL_MACHINE, keyname, 'Path', path);
  if path = '' then exit;
  if not CheckForMutexes('ClamWinTrayMutex01') then exit;

  if SuppressibleMsgBox(
      'The Setup detected that ClamWin is currently running.' + #13 +
      'Would you like to close it now? (Recommended)',
      mbConfirmation, MB_YESNO, IDYES) = idYes then
  begin
    path := RemoveQuotes(path) + '\WClose.exe';
    if FileExists(path) then
      Exec(path, '', '', SW_SHOWNORMAL, ewWaitUntilTerminated, resultcode);
  end;
end;

//────────────────────────────────────────────────────────────────────────────
// Wizard page skip logic
//────────────────────────────────────────────────────────────────────────────
function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if PageID = AllUsersPage.ID then
    Result := not IsAdminLoggedOn()
  else
    Result := False;
end;

//────────────────────────────────────────────────────────────────────────────
// Next-button click handler
//────────────────────────────────────────────────────────────────────────────
function NextButtonClick(CurPage: Integer): Boolean;
begin
  Result := True;
  if CurPage = wpFinished then
    SetupCompleted := True
  else if CurPage = AllUsersPage.ID then
  begin
    case AllUsersPage.SelectedValueIndex of
      0: AllUsers := True;
      1: AllUsers := False;
    end;
  end
  else if CurPage = wpReady then
    CloseClamTray();
end;

//────────────────────────────────────────────────────────────────────────────
// Store AllUsers choice for next run
//────────────────────────────────────────────────────────────────────────────
procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  case AllUsersPage.SelectedValueIndex of
    0: SetPreviousData(PreviousDataKey, 'AllUsersMode', 'anyone');
    1: SetPreviousData(PreviousDataKey, 'AllUsersMode', 'onlyme');
  end;
end;

//────────────────────────────────────────────────────────────────────────────
// Initialise
//────────────────────────────────────────────────────────────────────────────
function InitializeSetup(): Boolean;
var value: Cardinal;
begin
  Result      := True;
  AllUsers    := IsAdminLoggedOn();
  ThisVersion := 10501;   { 1.5.1 encoded as major*10000 + minor*100 + patch }
  Time        := GetDateTimeString('hh:nn', #0, ':') + ':00';

  value := 0;
  if not RegQueryDWordValue(HKEY_CURRENT_USER, 'SOFTWARE\ClamWin', 'Version', value) then
  begin
    value := 0;
    RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\ClamWin', 'Version', value);
  end;
  PreviousVersion := value;

  if PreviousVersion > ThisVersion then
  begin
    SuppressibleMsgBox(
      'Setup detected that a newer version of ClamWin is already installed.' + #13 +
      'Setup will now exit.',
      mbError, MB_OK, IDOK);
    Result := False;
  end;

  SetupCompleted := False;
end;

//────────────────────────────────────────────────────────────────────────────
// Wizard init — create the all-users selector page
//────────────────────────────────────────────────────────────────────────────
procedure InitializeWizard;
begin
  AllUsersPage := CreateInputOptionPage(wpLicense,
    'Select Installation Options',
    'Who should this application be installed for?',
    'Please select whether you wish to make this software available to all users or just yourself.',
    True, False);
  AllUsersPage.Add('&Anyone who uses this computer (all users)');
  AllUsersPage.Add('Only for &me (' + GetUserNameString() + ')');

  case GetPreviousData('AllUsersMode', '') of
    'anyone': AllUsers := True;
    'onlyme': AllUsers := False;
  end;
  if AllUsers then
    AllUsersPage.SelectedValueIndex := 0
  else
    AllUsersPage.SelectedValueIndex := 1;
end;

{ Returns the freshclam.conf path alongside ClamWin.conf in the profile dir. }
function FreshclamConfPath(Default: String): String;
begin
  Result := CommonProfileDir('') + '\.clamwin\freshclam.conf';
end;

{ Bootstrap a minimal freshclam.conf at install time so the [Run] freshclam
  invocation works even before the user opens preferences. }
procedure WriteFreshclamConf();
var
  DbDir, LogDir, ConfPath: String;
  Lines: TStringList;
begin
  DbDir    := CommonProfileDir('') + '\.clamwin\db';
  LogDir   := CommonProfileDir('') + '\.clamwin\log';
  ConfPath := FreshclamConfPath('');

  ForceDirectories(ExtractFileDir(ConfPath));

  Lines := TStringList.Create;
  try
    Lines.Add('# freshclam.conf — generated by ClamWin installer');
    Lines.Add('# Overwritten when preferences are saved.');
    Lines.Add('');
    Lines.Add('DatabaseMirror database.clamav.net');
    Lines.Add('DatabaseDirectory ' + DbDir);
    Lines.Add('UpdateLogFile ' + LogDir + '\ClamUpdateLog.txt');
    Lines.SaveToFile(ConfPath);
  finally
    Lines.Free;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    WriteFreshclamConf();
end;
