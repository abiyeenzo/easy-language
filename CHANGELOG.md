# Changelog

Toutes les versions notables du projet sont documentees ici.

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
