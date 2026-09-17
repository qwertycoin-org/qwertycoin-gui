#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef AppDisplayVersion
  #error AppDisplayVersion is required
#endif
#ifndef ArtifactName
  #error ArtifactName is required
#endif
#ifndef PackageDir
  #error PackageDir is required
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
#ifndef ProgramManifest
  #error ProgramManifest is required
#endif
#ifndef ProgramManifestSHA256
  #error ProgramManifestSHA256 is required
#endif
#ifndef ProgramFileList
  #error ProgramFileList is required
#endif
#ifndef SourceRevision
  #error SourceRevision is required
#endif
#ifndef CoreRevision
  #error CoreRevision is required
#endif
#ifndef RepositoryRoot
  #error RepositoryRoot is required
#endif

#define AppIdValue "{BEBB425B-5F3A-4F6C-AC09-DE09BE430880}"
#define InstallerRegistryKey "Software\Qwertycoin\Installer"
#define InstalledManifestName "program-files-v1.sha256"

[Setup]
AppId={#AppIdValue}
AppName=Qwertycoin
AppVersion={#AppVersion}
AppVerName=Qwertycoin {#AppDisplayVersion}
AppPublisher=The Qwertycoin Project
AppPublisherURL=https://qwertycoin.org/
AppSupportURL=https://github.com/qwertycoin-org/qwertycoin-gui/issues
AppUpdatesURL=https://github.com/qwertycoin-org/qwertycoin-gui/releases/latest
DefaultDirName={autopf}\Qwertycoin
DefaultGroupName=Qwertycoin
DisableProgramGroupPage=yes
AllowNoIcons=yes
UsePreviousAppDir=yes
UsePreviousGroup=yes
UsePreviousLanguage=yes
UsePreviousTasks=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
CloseApplicationsFilter=qwertycoin-gui.exe,qwertycoind.exe,qwertycoin-wallet-cli.exe,qwertycoin-wallet-rpc.exe
RestartApplications=no
RestartIfNeededByRun=no
SetupLogging=yes
WizardStyle=modern dynamic
WizardResizable=no
Compression=lzma2/ultra64
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename={#ArtifactName}-setup
SetupIconFile={#RepositoryRoot}\images\appicon.ico
UninstallDisplayIcon={app}\qwertycoin-gui.exe
UninstallDisplayName=Qwertycoin
UninstallFilesDir={app}\.qwertycoin-installer\uninstall
VersionInfoCompany=The Qwertycoin Project
VersionInfoDescription=Qwertycoin Wallet Installer
VersionInfoProductName=Qwertycoin
VersionInfoProductVersion={#AppVersion}
VersionInfoVersion={#AppVersion}.0
LicenseFile={#PackageDir}\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"

[CustomMessages]
english.DesktopIcon=Create a &desktop shortcut
german.DesktopIcon=&Desktopverknüpfung erstellen
english.DesktopTaskGroup=Additional shortcuts:
german.DesktopTaskGroup=Zusätzliche Verknüpfungen:
english.UnsafeExistingDirectory=The selected directory already contains files but is not a verified Qwertycoin installer directory. Select an empty program directory. No files were changed.%n%nDirectory: %1
german.UnsafeExistingDirectory=Der gewählte Ordner enthält bereits Dateien, ist aber kein verifiziertes Qwertycoin-Installationsverzeichnis. Wählen Sie einen leeren Programmordner. Es wurden keine Dateien geändert.%n%nOrdner: %1
english.TamperedInstallation=The existing Qwertycoin installation metadata is missing, inconsistent, or modified. Setup will not guess which files it owns. No files were changed.%n%n%1
german.TamperedInstallation=Die Metadaten der vorhandenen Qwertycoin-Installation fehlen, sind widersprüchlich oder wurden verändert. Das Setup wird nicht erraten, welche Dateien ihm gehören. Es wurden keine Dateien geändert.%n%n%1
english.DowngradeBlocked=A newer Qwertycoin version (%1) is already installed. Downgrade to %2 was blocked.
german.DowngradeBlocked=Eine neuere Qwertycoin-Version (%1) ist bereits installiert. Das Downgrade auf %2 wurde blockiert.
english.FileCollision=Setup found an existing file or directory that is not owned by the previous installer but conflicts with the new program package. Move it or select another program directory.%n%nConflict: %1
german.FileCollision=Das Setup hat eine vorhandene Datei oder einen Ordner gefunden, die bzw. der nicht dem vorherigen Installer gehört, aber mit dem neuen Programmpaket kollidiert. Verschieben Sie den Eintrag oder wählen Sie einen anderen Programmordner.%n%nKonflikt: %1
english.UnsafePath=Setup refused an unsafe path or reparse point. No files were changed.%n%nPath: %1
german.UnsafePath=Das Setup hat einen unsicheren Pfad oder Reparse Point abgelehnt. Es wurden keine Dateien geändert.%n%nPfad: %1
english.InstallVerificationFailed=The installed program files did not match the verified package. Setup is rolling back and no user data will be touched.%n%nFile: %1
german.InstallVerificationFailed=Die installierten Programmdateien stimmen nicht mit dem verifizierten Paket überein. Das Setup wird zurückgesetzt; Nutzerdaten werden nicht verändert.%n%nDatei: %1
english.RollbackConflict=Setup found an incomplete earlier update but cannot restore it without overwriting another file. No automatic cleanup was attempted.%n%nPath: %1
german.RollbackConflict=Das Setup hat ein unvollständiges früheres Update gefunden, kann es aber nicht wiederherstellen, ohne eine andere Datei zu überschreiben. Es wurde keine automatische Bereinigung versucht.%n%nPfad: %1

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; GroupDescription: "{cm:DesktopTaskGroup}"

[Dirs]
Name: "{app}\.qwertycoin-installer"

[Files]
; Every program file is generated from the already verified Windows package.
; ignoreversion is intentionally limited to these private application files so
; repair and upgrade always replace the complete package-owned program set.
#include ProgramFileList
Source: "{#ProgramManifest}"; Flags: dontcopy
Source: "{#ProgramManifest}"; DestDir: "{app}\.qwertycoin-installer"; DestName: "{#InstalledManifestName}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\Qwertycoin\Qwertycoin"; Filename: "{app}\qwertycoin-gui.exe"; WorkingDir: "{app}"; IconFilename: "{app}\qwertycoin-gui.exe"
Name: "{commondesktop}\Qwertycoin"; Filename: "{app}\qwertycoin-gui.exe"; WorkingDir: "{app}"; IconFilename: "{app}\qwertycoin-gui.exe"; Tasks: desktopicon

[Registry]
Root: HKLM64; Subkey: "{#InstallerRegistryKey}"; ValueType: string; ValueName: "AppId"; ValueData: "{#AppIdValue}"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "{#InstallerRegistryKey}"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "{#InstallerRegistryKey}"; ValueType: string; ValueName: "Version"; ValueData: "{#AppVersion}"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "{#InstallerRegistryKey}"; ValueType: string; ValueName: "ManifestSHA256"; ValueData: "{#ProgramManifestSHA256}"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "{#InstallerRegistryKey}"; ValueType: string; ValueName: "SourceRevision"; ValueData: "{#SourceRevision}"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "{#InstallerRegistryKey}"; ValueType: string; ValueName: "CoreRevision"; ValueData: "{#CoreRevision}"; Flags: uninsdeletekey

[Code]
const
  InstallerRegistryKey = '{#InstallerRegistryKey}';
  InstalledManifestName = '{#InstalledManifestName}';
  CurrentManifestSHA256 = '{#ProgramManifestSHA256}';
  CurrentVersion = '{#AppVersion}';
  ManifestHeader = '# qwertycoin-windows-program-manifest-v1';
  RollbackHeaderPrefix = '# old-manifest-sha256=';
  FileAttributeReparsePoint = $00000400;
  InvalidFileAttributes = $FFFFFFFF;

var
  CurrentFiles: TStringList;
  CurrentHashes: TStringList;
  OldFiles: TStringList;
  OldHashes: TStringList;
  ObsoleteFiles: TStringList;
  OldManifestSHA256: String;
  PreflightCompleted: Boolean;
  RollbackPrepared: Boolean;
  InstallSucceeded: Boolean;

function GetFileAttributesW(lpFileName: String): Cardinal;
  external 'GetFileAttributesW@kernel32.dll stdcall setuponly';

function MessageWithPath(const MessageName, Path: String): String;
begin
  Result := FmtMessage(ExpandConstant('{cm:' + MessageName + '}'), [Path]);
end;

function MetadataDirectory: String;
begin
  Result := AddBackslash(ExpandConstant('{app}')) + '.qwertycoin-installer';
end;

function InstalledManifestPath: String;
begin
  Result := AddBackslash(MetadataDirectory) + InstalledManifestName;
end;

function RollbackDirectory: String;
begin
  Result := AddBackslash(MetadataDirectory) + 'rollback-v1';
end;

function RollbackJournalPath: String;
begin
  Result := AddBackslash(RollbackDirectory) + 'paths.txt';
end;

function NativeRelativePath(const RelativePath: String): String;
begin
  Result := RelativePath;
  StringChangeEx(Result, '/', '\', True);
end;

function TargetPath(const RelativePath: String): String;
begin
  Result := AddBackslash(ExpandConstant('{app}')) + NativeRelativePath(RelativePath);
end;

function BackupPath(const RelativePath: String): String;
begin
  Result := AddBackslash(RollbackDirectory) + NativeRelativePath(RelativePath);
end;

function IsHexDigest(const Value: String): Boolean;
var
  Index: Integer;
begin
  Result := Length(Value) = 64;
  if not Result then
    exit;
  for Index := 1 to Length(Value) do
    if not (((Value[Index] >= '0') and (Value[Index] <= '9')) or
            ((Value[Index] >= 'a') and (Value[Index] <= 'f'))) then
    begin
      Result := False;
      exit;
    end;
end;

function IsDecimal(const Value: String): Boolean;
var
  Index: Integer;
begin
  Result := Value <> '';
  if not Result then
    exit;
  for Index := 1 to Length(Value) do
    if (Value[Index] < '0') or (Value[Index] > '9') then
    begin
      Result := False;
      exit;
    end;
end;

function IsReservedComponent(const Component: String): Boolean;
var
  BaseName: String;
  DotPosition: Integer;
begin
  BaseName := Uppercase(Component);
  DotPosition := Pos('.', BaseName);
  if DotPosition > 0 then
    BaseName := Copy(BaseName, 1, DotPosition - 1);
  Result := (BaseName = 'CON') or (BaseName = 'PRN') or
    (BaseName = 'AUX') or (BaseName = 'NUL') or
    (BaseName = 'COM1') or (BaseName = 'COM2') or
    (BaseName = 'COM3') or (BaseName = 'COM4') or
    (BaseName = 'COM5') or (BaseName = 'COM6') or
    (BaseName = 'COM7') or (BaseName = 'COM8') or (BaseName = 'COM9') or
    (BaseName = 'LPT1') or (BaseName = 'LPT2') or
    (BaseName = 'LPT3') or (BaseName = 'LPT4') or
    (BaseName = 'LPT5') or (BaseName = 'LPT6') or
    (BaseName = 'LPT7') or (BaseName = 'LPT8') or (BaseName = 'LPT9');
end;

function IsSafeRelativePath(const RelativePath: String): Boolean;
var
  Index: Integer;
  StartIndex: Integer;
  Component: String;
  Character: Char;
begin
  Result := False;
  if (RelativePath = '') or (RelativePath[1] = '/') or
     (RelativePath[1] = '\') or (Pos('\', RelativePath) > 0) or
     (Pos('//', RelativePath) > 0) then
    exit;

  StartIndex := 1;
  for Index := 1 to Length(RelativePath) + 1 do
  begin
    if (Index <= Length(RelativePath)) and (RelativePath[Index] <> '/') then
    begin
      Character := RelativePath[Index];
      if (Ord(Character) < 32) or (Pos(Character, '<>:"|?*;={}') > 0) then
        exit;
    end
    else
    begin
      Component := Copy(RelativePath, StartIndex, Index - StartIndex);
      if (Component = '') or (Component = '.') or (Component = '..') or
         (Component[Length(Component)] = ' ') or
         (Component[Length(Component)] = '.') or IsReservedComponent(Component) then
        exit;
      StartIndex := Index + 1;
    end;
  end;
  Result := True;
end;

function ContainsPath(Paths: TStringList; const RelativePath: String): Boolean;
var
  Index: Integer;
begin
  Result := False;
  for Index := 0 to Paths.Count - 1 do
    if CompareText(Paths[Index], RelativePath) = 0 then
    begin
      Result := True;
      exit;
    end;
end;

procedure LoadManifest(const FileName: String; Paths, Hashes: TStringList);
var
  Lines: TArrayOfString;
  Index: Integer;
  FirstTab: Integer;
  SecondTab: Integer;
  Rest: String;
  Digest: String;
  SizeText: String;
  RelativePath: String;
begin
  Paths.Clear;
  Hashes.Clear;
  if not LoadStringsFromFile(FileName, Lines) then
    RaiseException('Unable to read program manifest: ' + FileName);
  if (GetArrayLength(Lines) < 2) or (Lines[0] <> ManifestHeader) then
    RaiseException('Unsupported program manifest: ' + FileName);

  for Index := 1 to GetArrayLength(Lines) - 1 do
  begin
    FirstTab := Pos(#9, Lines[Index]);
    if FirstTab <= 1 then
      RaiseException('Malformed program manifest line.');
    Digest := Copy(Lines[Index], 1, FirstTab - 1);
    Rest := Copy(Lines[Index], FirstTab + 1, Length(Lines[Index]));
    SecondTab := Pos(#9, Rest);
    if SecondTab <= 1 then
      RaiseException('Malformed program manifest line.');
    SizeText := Copy(Rest, 1, SecondTab - 1);
    RelativePath := Copy(Rest, SecondTab + 1, Length(Rest));
    if not IsHexDigest(Digest) or not IsDecimal(SizeText) or
       not IsSafeRelativePath(RelativePath) then
      RaiseException('Unsafe program manifest entry: ' + RelativePath);
    if ContainsPath(Paths, RelativePath) then
      RaiseException('Duplicate program manifest entry: ' + RelativePath);
    if (Paths.Count > 0) and
       (CompareText(Paths[Paths.Count - 1], RelativePath) >= 0) then
      RaiseException('Program manifest is not canonically sorted.');
    Paths.Add(RelativePath);
    Hashes.Add(Digest);
  end;
  if Paths.Count = 0 then
    RaiseException('Program manifest is empty.');
end;

function DirectoryHasEntries(const Directory: String): Boolean;
var
  FindRecord: TFindRec;
begin
  Result := False;
  if not DirExists(Directory) then
    exit;
  if FindFirst(AddBackslash(Directory) + '*', FindRecord) then
  begin
    try
      repeat
        if (FindRecord.Name <> '.') and (FindRecord.Name <> '..') then
        begin
          Result := True;
          exit;
        end;
      until not FindNext(FindRecord);
    finally
      FindClose(FindRecord);
    end;
  end;
end;

function IsReparsePoint(const Path: String): Boolean;
var
  Attributes: Cardinal;
begin
  Attributes := GetFileAttributesW(Path);
  Result := (Attributes <> InvalidFileAttributes) and
    ((Attributes and FileAttributeReparsePoint) <> 0);
end;

procedure CheckNoReparseComponents(const RelativePath: String);
var
  CurrentPath: String;
  Remainder: String;
  SlashPosition: Integer;
  Component: String;
begin
  CurrentPath := RemoveBackslashUnlessRoot(ExpandConstant('{app}'));
  if IsReparsePoint(CurrentPath) then
    RaiseException(MessageWithPath('UnsafePath', CurrentPath));
  Remainder := RelativePath;
  while Remainder <> '' do
  begin
    SlashPosition := Pos('/', Remainder);
    if SlashPosition = 0 then
    begin
      Component := Remainder;
      Remainder := '';
    end
    else
    begin
      Component := Copy(Remainder, 1, SlashPosition - 1);
      Remainder := Copy(Remainder, SlashPosition + 1, Length(Remainder));
    end;
    CurrentPath := AddBackslash(CurrentPath) + Component;
    if IsReparsePoint(CurrentPath) then
      RaiseException(MessageWithPath('UnsafePath', CurrentPath));
    if (Remainder <> '') and FileExists(CurrentPath) then
      RaiseException(MessageWithPath('UnsafePath', CurrentPath));
  end;
end;

function ParseVersion(const Value: String; var Major, Minor, Patch: Integer): Boolean;
var
  FirstDot: Integer;
  SecondDot: Integer;
  Rest: String;
  MajorText: String;
  MinorText: String;
  PatchText: String;
begin
  Result := False;
  FirstDot := Pos('.', Value);
  if FirstDot <= 1 then
    exit;
  MajorText := Copy(Value, 1, FirstDot - 1);
  Rest := Copy(Value, FirstDot + 1, Length(Value));
  SecondDot := Pos('.', Rest);
  if SecondDot <= 1 then
    exit;
  MinorText := Copy(Rest, 1, SecondDot - 1);
  PatchText := Copy(Rest, SecondDot + 1, Length(Rest));
  if not IsDecimal(MajorText) or not IsDecimal(MinorText) or
     not IsDecimal(PatchText) then
    exit;
  Major := StrToInt(MajorText);
  Minor := StrToInt(MinorText);
  Patch := StrToInt(PatchText);
  Result := True;
end;

function CompareVersions(const Left, Right: String): Integer;
var
  LeftMajor, LeftMinor, LeftPatch: Integer;
  RightMajor, RightMinor, RightPatch: Integer;
begin
  if not ParseVersion(Left, LeftMajor, LeftMinor, LeftPatch) or
     not ParseVersion(Right, RightMajor, RightMinor, RightPatch) then
    RaiseException('Invalid installer version metadata.');
  if LeftMajor <> RightMajor then
    Result := LeftMajor - RightMajor
  else if LeftMinor <> RightMinor then
    Result := LeftMinor - RightMinor
  else
    Result := LeftPatch - RightPatch;
end;

procedure LoadRollbackJournal(Paths: TStringList; var PreviousDigest: String);
var
  Lines: TArrayOfString;
  Index: Integer;
begin
  Paths.Clear;
  PreviousDigest := '';
  if not LoadStringsFromFile(RollbackJournalPath, Lines) or
     (GetArrayLength(Lines) < 1) or
     (Copy(Lines[0], 1, Length(RollbackHeaderPrefix)) <> RollbackHeaderPrefix) then
    RaiseException(MessageWithPath('TamperedInstallation', RollbackJournalPath));
  PreviousDigest := Copy(
    Lines[0], Length(RollbackHeaderPrefix) + 1, Length(Lines[0]));
  if not IsHexDigest(PreviousDigest) then
    RaiseException(MessageWithPath('TamperedInstallation', RollbackJournalPath));
  for Index := 1 to GetArrayLength(Lines) - 1 do
  begin
    if not IsSafeRelativePath(Lines[Index]) or ContainsPath(Paths, Lines[Index]) then
      RaiseException(MessageWithPath('TamperedInstallation', RollbackJournalPath));
    Paths.Add(Lines[Index]);
  end;
end;

procedure RemoveEmptyRollbackDirectories(const RelativePath: String);
var
  Directory: String;
  StopDirectory: String;
begin
  Directory := ExtractFileDir(BackupPath(RelativePath));
  StopDirectory := RollbackDirectory;
  while (Length(Directory) > Length(StopDirectory)) and
        (CompareText(Copy(Directory, 1, Length(StopDirectory)), StopDirectory) = 0) do
  begin
    if not RemoveDir(Directory) then
      exit;
    Directory := ExtractFileDir(Directory);
  end;
end;

procedure DeleteRollbackCopies(Paths: TStringList);
var
  Index: Integer;
  StoredPath: String;
begin
  for Index := 0 to Paths.Count - 1 do
  begin
    StoredPath := BackupPath(Paths[Index]);
    if FileExists(StoredPath) then
    begin
      if IsReparsePoint(StoredPath) or not DeleteFile(StoredPath) then
        RaiseException(MessageWithPath('UnsafePath', StoredPath));
      RemoveEmptyRollbackDirectories(Paths[Index]);
    end;
  end;
  if FileExists(RollbackJournalPath) and not DeleteFile(RollbackJournalPath) then
    RaiseException(MessageWithPath('UnsafePath', RollbackJournalPath));
  RemoveDir(RollbackDirectory);
end;

procedure RestoreRollbackCopies(Paths: TStringList);
var
  Index: Integer;
  StoredPath: String;
  OriginalPath: String;
begin
  for Index := 0 to Paths.Count - 1 do
  begin
    StoredPath := BackupPath(Paths[Index]);
    OriginalPath := TargetPath(Paths[Index]);
    if FileExists(StoredPath) then
    begin
      if FileExists(OriginalPath) or DirExists(OriginalPath) then
        RaiseException(MessageWithPath('RollbackConflict', OriginalPath));
      if not ForceDirectories(ExtractFileDir(OriginalPath)) or
         not RenameFile(StoredPath, OriginalPath) then
        RaiseException(MessageWithPath('RollbackConflict', OriginalPath));
      RemoveEmptyRollbackDirectories(Paths[Index]);
    end;
  end;
  if FileExists(RollbackJournalPath) and not DeleteFile(RollbackJournalPath) then
    RaiseException(MessageWithPath('UnsafePath', RollbackJournalPath));
  RemoveDir(RollbackDirectory);
end;

procedure RecoverInterruptedCleanup;
var
  JournalPaths: TStringList;
  JournalOldDigest: String;
  RegisteredDigest: String;
  InstalledDigest: String;
begin
  if not FileExists(RollbackJournalPath) then
    exit;
  JournalPaths := TStringList.Create;
  try
    LoadRollbackJournal(JournalPaths, JournalOldDigest);
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE_64, InstallerRegistryKey,
      'ManifestSHA256', RegisteredDigest) or not FileExists(InstalledManifestPath) then
      RaiseException(MessageWithPath('TamperedInstallation', RollbackJournalPath));
    InstalledDigest := Lowercase(GetSHA256OfFile(InstalledManifestPath));
    if (InstalledDigest <> Lowercase(RegisteredDigest)) or
       not IsHexDigest(InstalledDigest) then
      RaiseException(MessageWithPath('TamperedInstallation', InstalledManifestPath));
    if InstalledDigest = JournalOldDigest then
      RestoreRollbackCopies(JournalPaths)
    else
      DeleteRollbackCopies(JournalPaths);
  finally
    JournalPaths.Free;
  end;
end;

procedure SaveRollbackJournal;
var
  Lines: TArrayOfString;
  Index: Integer;
begin
  if not ForceDirectories(RollbackDirectory) then
    RaiseException(MessageWithPath('UnsafePath', RollbackDirectory));
  SetArrayLength(Lines, ObsoleteFiles.Count + 1);
  Lines[0] := RollbackHeaderPrefix + OldManifestSHA256;
  for Index := 0 to ObsoleteFiles.Count - 1 do
    Lines[Index + 1] := ObsoleteFiles[Index];
  if not SaveStringsToFile(RollbackJournalPath, Lines, False) then
    RaiseException(MessageWithPath('UnsafePath', RollbackJournalPath));
end;

procedure StageObsoleteFiles;
var
  Index: Integer;
  OriginalPath: String;
  StoredPath: String;
begin
  if ObsoleteFiles.Count = 0 then
    exit;
  SaveRollbackJournal;
  RollbackPrepared := True;
  for Index := 0 to ObsoleteFiles.Count - 1 do
  begin
    OriginalPath := TargetPath(ObsoleteFiles[Index]);
    if FileExists(OriginalPath) then
    begin
      StoredPath := BackupPath(ObsoleteFiles[Index]);
      if FileExists(StoredPath) or DirExists(StoredPath) or
         not ForceDirectories(ExtractFileDir(StoredPath)) or
         not RenameFile(OriginalPath, StoredPath) then
        RaiseException(MessageWithPath('RollbackConflict', OriginalPath));
    end;
  end;
end;

procedure VerifyInstalledProgramFiles;
var
  Index: Integer;
  FileName: String;
begin
  if not FileExists(InstalledManifestPath) or
     (Lowercase(GetSHA256OfFile(InstalledManifestPath)) <> CurrentManifestSHA256) then
    RaiseException(MessageWithPath('InstallVerificationFailed', InstalledManifestPath));
  for Index := 0 to CurrentFiles.Count - 1 do
  begin
    FileName := TargetPath(CurrentFiles[Index]);
    if not FileExists(FileName) or IsReparsePoint(FileName) or
       (Lowercase(GetSHA256OfFile(FileName)) <> CurrentHashes[Index]) then
      RaiseException(MessageWithPath('InstallVerificationFailed', FileName));
  end;
end;

procedure PerformPreflight;
var
  CurrentManifestPath: String;
  AppDirectory: String;
  RegisteredInstallDir: String;
  RegisteredVersion: String;
  Index: Integer;
  FileName: String;
begin
  CurrentManifestPath := ExpandConstant('{tmp}\{#InstalledManifestName}');
  ExtractTemporaryFile('{#InstalledManifestName}');
  if not FileExists(CurrentManifestPath) or
     (Lowercase(GetSHA256OfFile(CurrentManifestPath)) <> CurrentManifestSHA256) then
    RaiseException(MessageWithPath('TamperedInstallation', CurrentManifestPath));
  LoadManifest(CurrentManifestPath, CurrentFiles, CurrentHashes);

  AppDirectory := RemoveBackslashUnlessRoot(ExpandConstant('{app}'));
  if IsReparsePoint(AppDirectory) then
    RaiseException(MessageWithPath('UnsafePath', AppDirectory));
  RecoverInterruptedCleanup;

  if DirectoryHasEntries(AppDirectory) then
  begin
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE_64, InstallerRegistryKey,
       'InstallDir', RegisteredInstallDir) or
       (CompareText(RemoveBackslashUnlessRoot(RegisteredInstallDir), AppDirectory) <> 0) or
       not RegQueryStringValue(HKEY_LOCAL_MACHINE_64, InstallerRegistryKey,
       'ManifestSHA256', OldManifestSHA256) or
       not IsHexDigest(Lowercase(OldManifestSHA256)) or
       not FileExists(InstalledManifestPath) or
       (Lowercase(GetSHA256OfFile(InstalledManifestPath)) <> Lowercase(OldManifestSHA256)) then
      RaiseException(MessageWithPath('UnsafeExistingDirectory', AppDirectory));

    OldManifestSHA256 := Lowercase(OldManifestSHA256);
    LoadManifest(InstalledManifestPath, OldFiles, OldHashes);
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE_64, InstallerRegistryKey,
       'Version', RegisteredVersion) then
      RaiseException(MessageWithPath('TamperedInstallation', AppDirectory));
    if CompareVersions(RegisteredVersion, CurrentVersion) > 0 then
      RaiseException(FmtMessage(ExpandConstant('{cm:DowngradeBlocked}'),
        [RegisteredVersion, CurrentVersion]));
  end
  else
  begin
    OldManifestSHA256 := '';
    OldFiles.Clear;
    OldHashes.Clear;
  end;

  for Index := 0 to CurrentFiles.Count - 1 do
  begin
    CheckNoReparseComponents(CurrentFiles[Index]);
    FileName := TargetPath(CurrentFiles[Index]);
    if (FileExists(FileName) or DirExists(FileName)) and
       not ContainsPath(OldFiles, CurrentFiles[Index]) then
      RaiseException(MessageWithPath('FileCollision', FileName));
    if DirExists(FileName) then
      RaiseException(MessageWithPath('FileCollision', FileName));
  end;

  ObsoleteFiles.Clear;
  for Index := 0 to OldFiles.Count - 1 do
  begin
    CheckNoReparseComponents(OldFiles[Index]);
    FileName := TargetPath(OldFiles[Index]);
    if DirExists(FileName) then
      RaiseException(MessageWithPath('UnsafePath', FileName));
    if not ContainsPath(CurrentFiles, OldFiles[Index]) then
      ObsoleteFiles.Add(OldFiles[Index]);
  end;
  PreflightCompleted := True;
end;

function InitializeSetup: Boolean;
begin
  CurrentFiles := TStringList.Create;
  CurrentHashes := TStringList.Create;
  OldFiles := TStringList.Create;
  OldHashes := TStringList.Create;
  ObsoleteFiles := TStringList.Create;
  Result := True;
end;

procedure InitializeWizard;
begin
  { Qwertycoin gold accents without an external skin engine. }
  WizardForm.WelcomeLabel1.Font.Color := $003CB7F6;
  WizardForm.PageNameLabel.Font.Color := $003CB7F6;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  try
    PerformPreflight;
  except
    Result := GetExceptionMessage;
  end;
end;

procedure RegisterExtraCloseApplicationsResources;
var
  Index: Integer;
  FileName: String;
begin
  if not PreflightCompleted then
    exit;
  for Index := 0 to ObsoleteFiles.Count - 1 do
  begin
    FileName := TargetPath(ObsoleteFiles[Index]);
    if FileExists(FileName) then
      RegisterExtraCloseApplicationsResource(FileName);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    VerifyInstalledProgramFiles;
    StageObsoleteFiles;
  end
  else if CurStep = ssDone then
  begin
    InstallSucceeded := True;
    if RollbackPrepared then
      DeleteRollbackCopies(ObsoleteFiles);
    RollbackPrepared := False;
  end;
end;

procedure DeinitializeSetup;
begin
  if RollbackPrepared and not InstallSucceeded then
  begin
    try
      RestoreRollbackCopies(ObsoleteFiles);
    except
      Log('Rollback recovery remains pending: ' + GetExceptionMessage);
    end;
  end;
  CurrentFiles.Free;
  CurrentHashes.Free;
  OldFiles.Free;
  OldHashes.Free;
  ObsoleteFiles.Free;
end;
