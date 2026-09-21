# Packaging Windows

## Automatique (recommande)

Le workflow `.github/workflows/build-windows.yml` tourne sur `windows-latest`
et fait tout automatiquement : tests, `easy_language.exe`,
`easy_language_editeur.exe`, et `EasyLanguageSetup.exe` (installateur Inno
Setup avec raccourcis + association `.elg`).

- Sur chaque push/PR : build de verification (artefacts telechargeables
  depuis l'onglet "Actions" du depot, colonne "Artifacts").
- Sur un tag `vX.Y.Z` pousse sur GitHub : cree en plus une **Release**
  GitHub avec les 3 fichiers attaches, prets a etre lies depuis un bouton
  de telechargement sur le site.

Pour declencher une release :
```
git tag v1.0.0
git push origin v1.0.0
```

## Manuel (sur une machine Windows, pour tester avant de tagger)

```
pip install pyinstaller
pyinstaller --onefile --name easy_language main.py
pyinstaller --onefile --windowed --name easy_language_editeur editeur.py

REM necessite Inno Setup installe (https://jrsoftware.org/isdl.php)
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" packaging\installateur.iss
```

Resultats : `dist\easy_language.exe`, `dist\easy_language_editeur.exe`,
`Output\EasyLanguageSetup.exe`.
