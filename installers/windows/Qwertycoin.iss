; Qwertycoin GUI Wallet Installer for Windows
; Copyright (c) 2017-2024, The Monero Project
; Copyright (c) 2026, The Qwertycoin Project
; See LICENSE

#define GuiVersion GetFileVersion("bin\qwertycoin-gui.exe")

[Setup]
AppId={{0E1A46D4-5D6E-4D9D-B97B-1B1E348E6F2D}
AppName=Qwertycoin GUI
AppVersion={#GuiVersion}
VersionInfoVersion={#GuiVersion}
DefaultDirName={localappdata}\Programs\Qwertycoin GUI
DefaultGroupName=Qwertycoin GUI
UninstallDisplayIcon={app}\qwertycoin-gui.exe
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64 arm64
WizardSmallImageFile=WizardSmallImage.bmp
WizardImageFile=WelcomeImage.bmp
DisableWelcomePage=no
LicenseFile=LICENSE
AppPublisher=The Qwertycoin Project
AppPublisherURL=https://qwertycoin.org
AppSupportURL=https://github.com/qwertycoin-org/qwertycoin-gui/issues
AppUpdatesURL=https://qwertycoin.org
TimeStampsInUTC=yes
Compression=lzma2/ultra64
SolidCompression=yes

[Messages]
SetupWindowTitle=%1 {#GuiVersion} Installer

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "bin\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "ReadMe.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "qwertycoin-daemon.bat"; DestDir: "{app}"; Flags: ignoreversion

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"

[Icons]
Name: "{group}\Qwertycoin GUI"; Filename: "{app}\qwertycoin-gui.exe"
Name: "{group}\Qwertycoin Daemon"; Filename: "{app}\qwertycoin-daemon.bat"; WorkingDir: "{app}"
Name: "{group}\Qwertycoin CLI Wallet"; Filename: "{app}\qwertycoin-wallet-cli.exe"; WorkingDir: "{userdocs}\Qwertycoin\wallets"
Name: "{group}\Read Me"; Filename: "{app}\ReadMe.htm"
Name: "{group}\Uninstall Qwertycoin GUI"; Filename: "{uninstallexe}"
Name: "{userdesktop}\Qwertycoin GUI"; Filename: "{app}\qwertycoin-gui.exe"; Tasks: desktopicon

[Registry]
; Wallet/configuration directories are intentionally not removed on uninstall.
Root: HKCR; Subkey: "qwertycoin"; ValueType: string; ValueData: "URL:Qwertycoin Payment Protocol"; Flags: uninsdeletekey
Root: HKCR; Subkey: "qwertycoin"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCR; Subkey: "qwertycoin\DefaultIcon"; ValueType: string; ValueData: "{app}\qwertycoin-gui.exe,0"
Root: HKCR; Subkey: "qwertycoin\shell\open\command"; ValueType: string; ValueData: """{app}\qwertycoin-gui.exe"" ""%1"""

[Run]
Filename: "{app}\ReadMe.htm"; Description: "Show ReadMe"; Flags: postinstall shellexec skipifsilent
