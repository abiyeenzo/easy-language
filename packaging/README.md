# Packaging Windows

Le langage est ecrit en C pur (voir `c/`), compile par cross-compilation
avec `mingw-w64` (`x86_64-w64-mingw32-gcc`). Aucun runner Windows n'est
necessaire pour produire les `.exe` : GCC/mingw supporte la
cross-compilation reelle depuis Linux, contrairement a un empaqueteur
comme PyInstaller.

## Automatique (recommande)

Le workflow `.github/workflows/build-windows.yml` tourne sur
`ubuntu-latest`, installe `gcc-mingw-w64-x86-64`, compile et verifie
l'interpreteur natif (Linux), puis cross-compile les trois executables
Windows :

- `easy_language.exe` (interpreteur, ligne de commande)
- `easy_debogueur.exe` (debogueur pas-a-pas, ligne de commande)
- `easy_editeur.exe` (editeur graphique Win32)

Le tout est zippe avec les exemples et la documentation dans
`EasyLanguage-windows.zip`.

Un second job (`installateur`, sur un runner `windows-latest` avec Inno
Setup via l'action `Minionguyjpro/Inno-Setup-Action`) recupere ces `.exe`
et compile `packaging/installateur.iss` en un installateur unique
`EasyLanguage-installateur.exe` : menu Demarrer, raccourci Bureau
optionnel, association `.elg` (icone + ouverture dans l'editeur), et
ajout optionnel au `PATH` utilisateur pour utiliser `easy_language` /
`easy_debogueur` depuis n'importe quel `cmd`/PowerShell. Aucun droit
administrateur requis (installation dans le profil utilisateur).

- Sur chaque push/PR : build de verification (artefacts telechargeables
  depuis l'onglet "Actions" du depot).
- Sur un tag `vX.Y.Z` pousse sur GitHub : cree en plus une **Release**
  GitHub avec le zip, les `.exe` individuels et l'installateur attaches,
  prets a etre lies depuis des boutons de telechargement sur le site.

Pour declencher une release :
```
git tag v1.0.0
git push origin v1.0.0
```

## Manuel (depuis Linux, ou Windows avec MinGW/MSYS2)

```
cd c
make windows
```

Resultats : `c/easy_language.exe`, `c/easy_debogueur.exe`,
`c/easy_editeur.exe`. Les trois doivent rester dans le meme dossier :
l'editeur lance les deux autres via `CreateProcess` en supposant qu'ils
sont a cote de lui.

## Installateur (Windows uniquement)

Inno Setup ne tourne que sous Windows (ou via Wine) : `installateur.iss`
ne peut pas etre compile depuis cet environnement Linux, contrairement
au reste du projet. Pour le compiler manuellement :

1. Compiler les trois `.exe` (`cd c && make windows`, depuis Windows
   avec MinGW/MSYS2, ou recuperer ceux publies par la CI).
2. Installer [Inno Setup](https://jrsoftware.org/isinfo.php).
3. Ouvrir `packaging/installateur.iss` dans Inno Setup, ou en ligne de
   commande : `ISCC installateur.iss`.

Resultat : `dist_installateur/EasyLanguage-installateur.exe`.

## Verification locale (Linux, avant de cross-compiler)

```
cd c
make linux
./easy_language ../exemples/bonjour.elg
./easy_debogueur ../exemples/fibonacci.elg 5
```

`easy_editeur.exe` utilise l'API Win32 directement (RichEdit,
CreateProcess) : il ne se compile que pour Windows, et n'a pas pu etre
teste visuellement depuis cet environnement Linux (pas de Windows/Wine
disponible ici). Sa compilation croisee reussit sans erreur ni warning,
ce qui donne une bonne confiance, mais un test manuel sur une vraie
machine Windows reste recommande avant de s'y fier pour un rendu final.
