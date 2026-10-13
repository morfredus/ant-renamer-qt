; Installeur Ant Renamer Qt (Inno Setup 6). Lance par scripts\windows\package-win.ps1, qui definit :
;   /DAppVersion=3.0.0-rc.1  /DAppVersionNum=3.0.0.0  /DStageDir=<dossier a installer>

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef AppVersionNum
  #define AppVersionNum "0.0.0.0"
#endif
#ifndef StageDir
  #error StageDir doit etre defini (voir package-win.ps1)
#endif

[Setup]
AppId={{8B3C2F0A-5D7E-4C1B-9A62-3E4F7D1A9C20}
AppName=Ant Renamer Qt
AppVersion={#AppVersion}
AppVerName=Ant Renamer Qt {#AppVersion}
AppPublisher=Antoine Potten, Frédéric Biron (port C++/Qt)
AppPublisherURL=https://www.antp.be/software/renamer/
AppCopyright=Copyright (C) 2000-2025 Antoine Potten, port C++/Qt Frédéric Biron
VersionInfoVersion={#AppVersionNum}
DefaultDirName={autopf}\Ant Renamer Qt
DefaultGroupName=Ant Renamer Qt
AllowNoIcons=yes
LicenseFile={#StageDir}\license.txt
UninstallDisplayIcon={app}\antrenamer-qt.exe
OutputBaseFilename=AntRenamerQt-{#AppVersion}-win64-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Installation par utilisateur possible sans droits administrateur (choix propose au lancement).
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Ant Renamer Qt"; Filename: "{app}\antrenamer-qt.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\Ant Renamer Qt"; Filename: "{app}\antrenamer-qt.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "shellmenu"; Description: "{cm:ShellMenuTask}"; GroupDescription: "{cm:ShellMenuGroup}"

[CustomMessages]
english.ShellMenuGroup=Windows Explorer
english.ShellMenuTask=Add "Add to Ant Renamer Qt" to the context menu of files and folders
english.ShellMenu=Add to Ant Renamer Qt
french.ShellMenuGroup=Explorateur Windows
french.ShellMenuTask=Ajouter « Ajouter à Ant Renamer Qt » au menu contextuel des fichiers et dossiers
french.ShellMenu=Ajouter à Ant Renamer Qt

[Registry]
; Meme entrees que la case des Options du programme (HKCU\Software\Classes\{*,Directory}\shell\AntRenamerQt).
; Windows 11 : l'entree est dans « Afficher plus d'options » (menu classique).
Root: HKA; Subkey: "Software\Classes\*\shell\AntRenamerQt"; ValueType: string; ValueName: ""; ValueData: "{cm:ShellMenu}"; Flags: uninsdeletekey; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\*\shell\AntRenamerQt"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\antrenamer-qt.exe"",0"; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\*\shell\AntRenamerQt\command"; ValueType: string; ValueName: ""; ValueData: """{app}\antrenamer-qt.exe"" --shell ""%1"""; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\AntRenamerQt"; ValueType: string; ValueName: ""; ValueData: "{cm:ShellMenu}"; Flags: uninsdeletekey; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\AntRenamerQt"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\antrenamer-qt.exe"",0"; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\AntRenamerQt\command"; ValueType: string; ValueName: ""; ValueData: """{app}\antrenamer-qt.exe"" --shell ""%1"""; Tasks: shellmenu

[Run]
Filename: "{app}\antrenamer-qt.exe"; Description: "{cm:LaunchProgram,Ant Renamer Qt}"; Flags: nowait postinstall skipifsilent

; Les reglages de l'utilisateur (%APPDATA%\AntRenamerQt) sont volontairement conserves a la desinstallation.
