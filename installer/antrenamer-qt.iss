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
AppCopyright=Copyright (C) 2000-2026 Antoine Potten - Frédéric Biron, port C++/Qt Frédéric Biron
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
Name: "updatecheck"; Description: "{cm:UpdateCheckTask}"; GroupDescription: "{cm:UpdateCheckGroup}"; Flags: unchecked
Name: "prereleases"; Description: "{cm:PrereleasesTask}"; GroupDescription: "{cm:UpdateCheckGroup}"; Flags: unchecked

[CustomMessages]
english.ShellMenuGroup=Windows Explorer
english.ShellMenuTask=Add "Add to Ant Renamer Qt" to the context menu of files and folders
english.ShellMenu=Add to Ant Renamer Qt
french.ShellMenuGroup=Explorateur Windows
french.ShellMenuTask=Ajouter « Ajouter à Ant Renamer Qt » au menu contextuel des fichiers et dossiers
french.ShellMenu=Ajouter à Ant Renamer Qt
english.UpdateCheckGroup=New-version check
english.UpdateCheckTask=Look for a new version at start (once a day, asks GitHub; nothing else is sent)
english.PrereleasesTask=Also tell me about beta and release candidates (otherwise final versions only)
french.UpdateCheckGroup=Recherche de nouvelle version
french.UpdateCheckTask=Rechercher une nouvelle version au démarrage (une fois par jour, interroge GitHub ; rien d'autre n'est envoyé)
french.PrereleasesTask=Me signaler aussi les versions bêta et release candidates (sinon, versions finales seulement)

[Registry]
; Meme entrees que la case des Options du programme (HKCU\Software\Classes\{*,Directory}\shell\AntRenamerQt).
; Windows 11 : l'entree est dans « Afficher plus d'options » (menu classique).
Root: HKA; Subkey: "Software\Classes\*\shell\AntRenamerQt"; ValueType: string; ValueName: ""; ValueData: "{cm:ShellMenu}"; Flags: uninsdeletekey; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\*\shell\AntRenamerQt"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\antrenamer-qt.exe"",0"; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\*\shell\AntRenamerQt\command"; ValueType: string; ValueName: ""; ValueData: """{app}\antrenamer-qt.exe"" --shell ""%1"""; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\AntRenamerQt"; ValueType: string; ValueName: ""; ValueData: "{cm:ShellMenu}"; Flags: uninsdeletekey; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\AntRenamerQt"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\antrenamer-qt.exe"",0"; Tasks: shellmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\AntRenamerQt\command"; ValueType: string; ValueName: ""; ValueData: """{app}\antrenamer-qt.exe"" --shell ""%1"""; Tasks: shellmenu

[Dirs]
; Dossier des reglages (QSettings : %APPDATA%\AntRenamerQt\Ant Renamer Qt), cree pour y ecrire les deux choix ci-dessous.
Name: "{userappdata}\AntRenamerQt\Ant Renamer Qt"; Flags: uninsneveruninstall; Tasks: updatecheck prereleases

[INI]
; Memes cles que les cases des Options (section options). Une case decochee n'ecrit rien : un choix deja fait dans le programme est conserve.
Filename: "{userappdata}\AntRenamerQt\Ant Renamer Qt\antrenamer-qt.ini"; Section: "options"; Key: "checkUpdates"; String: "true"; Tasks: updatecheck
Filename: "{userappdata}\AntRenamerQt\Ant Renamer Qt\antrenamer-qt.ini"; Section: "options"; Key: "checkPrereleases"; String: "true"; Tasks: prereleases

[Run]
Filename: "{app}\antrenamer-qt.exe"; Description: "{cm:LaunchProgram,Ant Renamer Qt}"; Flags: nowait postinstall skipifsilent

; Les reglages de l'utilisateur (%APPDATA%\AntRenamerQt) sont volontairement conserves a la desinstallation.
