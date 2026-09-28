; Script Inno Setup pour Easy Language.
; Compilation : ISCC installateur.iss  (depuis Windows, ou via l'action
; GitHub Actions Minionguyjpro/Inno-Setup-Action sur un runner windows-latest).
; Suppose que c\easylang.exe, c\easy_debogueur.exe et c\easy_editeur.exe
; sont deja compiles (cf. packaging/README.md, cible "make windows").

#define MyAppName "Easy Language"
#define MyAppVersion "1.0.9"
#define MyAppPublisher "AE Corporation"
#define MyAppURL "https://github.com/abiyeenzo/easy-language"
#define MyAppExeEditeur "easy_editeur.exe"

[Setup]
AppId={{8F2C1E44-6B7A-4C9D-9E3F-2B6A7D4C8E10}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright=Copyright (C) 2026 Abiye Enzo. Tous droits reserves.
; Installation dans le profil utilisateur : aucun droit administrateur
; requis, coherent avec l'association de fichiers (HKCU) faite plus bas.
DefaultDirName={userpf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=..\dist_installateur
OutputBaseFilename=EasyLanguage-installateur
SetupIconFile=icone.ico
UninstallDisplayIcon={app}\{#MyAppExeEditeur}
; Page dediee affichee avant l'installation (auteur, societe, depot) :
; voir packaging/a_propos_installateur.txt.
InfoBeforeFile=a_propos_installateur.txt
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "desktopicon"; Description: "Creer un raccourci sur le Bureau"; GroupDescription: "Raccourcis supplementaires :"
Name: "envpath"; Description: "Ajouter Easy Language au PATH (utiliser easylang / easy_debogueur depuis l'invite de commandes)"; GroupDescription: "Options :"; Flags: checkedonce

; Nettoie l'ancien nom de l'interpreteur (easy_language.exe, renomme en
; easylang.exe) lors d'une mise a jour depuis une version anterieure ;
; sans quoi Inno Setup laisserait ce fichier orphelin sur le disque, ne
; faisant plus partie du manifeste [Files] actuel.
[InstallDelete]
Type: files; Name: "{app}\easy_language.exe"

[Files]
Source: "..\c\easylang.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\c\easy_debogueur.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\c\easy_editeur.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "icone.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\exemples\*"; DestDir: "{app}\exemples"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\bibliotheque\*"; DestDir: "{app}\bibliotheque"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\GUIDE_UTILISATEUR.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Easy Language - Editeur"; Filename: "{app}\{#MyAppExeEditeur}"; IconFilename: "{app}\icone.ico"
Name: "{group}\Guide utilisateur"; Filename: "{app}\GUIDE_UTILISATEUR.md"
Name: "{group}\Desinstaller {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Easy Language - Editeur"; Filename: "{app}\{#MyAppExeEditeur}"; IconFilename: "{app}\icone.ico"; Tasks: desktopicon

; Meme association HKCU (par utilisateur, sans droit admin) que
; packaging/associer_fichiers.ps1, mais geree automatiquement par
; l'installateur/desinstalleur (uninsdeletekey/uninsdeletevalue).
[Registry]
Root: HKCU; Subkey: "Software\Classes\.elg"; ValueType: string; ValueName: ""; ValueData: "EasyLanguage.Script"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\EasyLanguage.Script"; ValueType: string; ValueName: ""; ValueData: "Script Easy Language"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\EasyLanguage.Script\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\icone.ico"""
Root: HKCU; Subkey: "Software\Classes\EasyLanguage.Script\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeEditeur}"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeEditeur}"; Description: "Lancer Easy Language - Editeur"; Flags: nowait postinstall skipifsilent unchecked

[Code]
const
  EnvironmentKey = 'Environment';

{ Ajoute Path a la variable PATH de l'utilisateur (HKCU), sans doublon. }
procedure EnvAddPath(Path: string);
var
  Paths: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths) then
    Paths := '';
  if Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';') > 0 then
    exit;
  if (Length(Paths) > 0) and (Paths[Length(Paths)] <> ';') then
    Paths := Paths + ';';
  Paths := Paths + Path + ';';
  RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths);
end;

{ Retire Path de la variable PATH de l'utilisateur, a la desinstallation. }
procedure EnvRemovePath(Path: string);
var
  Paths: string;
  P: Integer;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths) then
    exit;
  P := Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';');
  if P = 0 then
    exit;
  Delete(Paths, P - 1, Length(Path) + 1);
  RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('envpath') then
    EnvAddPath(ExpandConstant('{app}'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    EnvRemovePath(ExpandConstant('{app}'));
end;
