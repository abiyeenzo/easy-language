; Script Inno Setup pour Easy Language.
; Compiler avec: ISCC.exe packaging\installateur.iss (execute depuis la
; racine du depot). SourceDir=.. remonte les chemins "Source:" a la racine
; du projet (ou se trouvent dist/, exemples/ et README.md); OutputDir est
; relatif au repertoire courant d'ISCC (la racine du depot).

#define MyAppName "Easy Language"
#define MyAppVersion "1.0"
#define MyAppPublisher "Easy Language"

[Setup]
AppId={{6E6E6F76-656E-4F5A-B1BE-45415359454C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\EasyLanguage
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
SourceDir=..
OutputDir=Output
OutputBaseFilename=EasyLanguageSetup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\easy_language_editeur.exe

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Files]
Source: "dist\easy_language.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "dist\easy_language_editeur.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "exemples\*"; DestDir: "{app}\exemples"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Easy Language - Editeur"; Filename: "{app}\easy_language_editeur.exe"
Name: "{group}\Exemples"; Filename: "{app}\exemples"
Name: "{group}\Desinstaller {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Easy Language"; Filename: "{app}\easy_language_editeur.exe"; Tasks: raccourcibureau

[Tasks]
Name: "raccourcibureau"; Description: "Creer un raccourci sur le Bureau"; GroupDescription: "Raccourcis supplementaires"

[Registry]
Root: HKCR; Subkey: ".elg"; ValueType: string; ValueName: ""; ValueData: "EasyLanguageScript"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "EasyLanguageScript"; ValueType: string; ValueName: ""; ValueData: "Script Easy Language"; Flags: uninsdeletekey
Root: HKCR; Subkey: "EasyLanguageScript\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\easy_language_editeur.exe,0"
Root: HKCR; Subkey: "EasyLanguageScript\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\easy_language_editeur.exe"" ""%1"""
Root: HKCR; Subkey: "EasyLanguageScript\shell\executer"; ValueType: string; ValueName: ""; ValueData: "Executer avec Easy Language"
Root: HKCR; Subkey: "EasyLanguageScript\shell\executer\command"; ValueType: string; ValueName: ""; ValueData: """{app}\easy_language.exe"" ""%1"""

[Run]
Filename: "{app}\easy_language_editeur.exe"; Description: "Lancer l'editeur Easy Language"; Flags: nowait postinstall skipifsilent
