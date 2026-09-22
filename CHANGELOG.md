# Changelog

Toutes les versions notables du projet sont documentees ici.

## [Non publie]

### Ajoute
- `demande` accepte desormais une syntaxe avec parentheses,
  `demande(nom, "invite")`, en plus de l'ancienne forme `demande nom
  "invite"` (toujours valide).
- Virgule optionnelle comme separateur dans les litteraux de liste, les
  arguments d'appel de fonction et les parametres de definition de
  fonction (`[1, 2, 3]`, `fonction f(a, b):`, `f(1, 2)`).

### Corrige
- Editeur graphique : le champ d'entree standard etait un buffer a
  pre-remplir avant de lancer le script (F5), ferme des le lancement.
  Si une valeur manquait, `demande` lisait une entree vide, causant des
  variables vides puis des erreurs de type sur les comparaisons
  (`l'operateur '>=' attend des nombres`). Le champ d'entree reste
  maintenant ouvert pendant toute l'execution (comme pour le
  debogueur) : l'utilisateur tape sa reponse au moment ou `demande`
  l'attend reellement, puis Entree.

## [2.0.0]

Reecriture complete du langage en C (aucune dependance a Python n'est
conservee dans le rendu).

### Change
- Interpreteur, debogueur et editeur graphique reecrits en C pur
  (`c/`), avec des points chauds en assembleur x86-64 inline pour
  l'arithmetique entiere. ~30x plus rapide que l'ancienne version
  Python sur un benchmark `fib(30)` recursif.
- L'editeur graphique passe de tkinter (Python) a l'API Win32 native
  (RichEdit pour la coloration syntaxique, `CreateProcess`/pipes pour
  Lancer/Deboguer).
- Packaging simplifie : cross-compilation directe vers `.exe` Windows
  via `mingw-w64`, sans runner Windows ni PyInstaller.
- Nouvelle suite de tests en C (`c/tests/`, 81 verifications) qui pilote
  les executables compiles comme boite noire, remplace l'ancienne suite
  `unittest` Python.

### Retire
- Systeme de modules (`importe ... comme ...`) : pas encore porte en C.

## [1.0.0]

Premiere version.

### Ajoute
- Interpreteur du langage (`main.py`) : lexer, analyseur, evaluateur.
- Syntaxe : variables, conditions (`si`/`sinon si`/`sinon`), boucles
  (`tantque`, `pour ... de ... jusqua ... pas`, `pour chaque ... dans`),
  fonctions (recursion, portee locale), `arrete`/`continue`.
- Listes/tableaux : litteraux `[1 2 3]`, indexation, natives `ajoute`,
  `retire`, `contient`, `longueur`, `nombre`, `entier`, `texte`.
- Systeme de modules (`importe "fichier.elg" comme alias`), avec
  detection d'import circulaire.
- Debogueur pas-a-pas (`debogueur_cli.py`) : points d'arret, execution
  ligne par ligne, inspection de variables.
- Editeur graphique (`editeur.py`) : coloration syntaxique, execution
  (F5) et debogage (F6) integres.
- Suite de tests unitaires (102 tests, `unittest` de la stdlib).
- Packaging Windows : PyInstaller + installateur Inno Setup, construits
  automatiquement par une CI GitHub Actions a chaque tag `vX.Y.Z`.
- Site de presentation et de telechargement (`site/index.html`).
