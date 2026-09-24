# Changelog

Toutes les versions notables du projet sont documentees ici.

## [1.0.5] - 2026-09-24

### Ajoute
- Bibliotheque standard (`bibliotheque/`), equivalent reduit des modules
  Python `math`, `os` et `socket`, plus des boites de dialogue simples :
  - `math.elg` : racine, puissance, sin/cos/tan, abs, plancher/plafond/
    arrondi, log/exp, alea/alea_entier, min/max, constantes PI et E.
  - `os.elg` : fichier_existe, lire_fichier, ecrire_fichier,
    ajouter_fichier, supprimer_fichier, repertoire_courant,
    variable_environnement, horodatage, dormir.
  - `reseau.elg` : client TCP basique (connecter/envoyer/recevoir/fermer).
  - `gui.elg` : message()/question(), vraies boites Win32 (MessageBoxW)
    sous Windows, repli console ailleurs pour rester testable partout.
  Implementees comme fonctions natives en C (`c/natifs_math.c`,
  `c/natifs_os.c`, `c/natifs_reseau.c`, `c/natifs_gui.c`) enveloppees par
  des modules `.elg` pour l'acces namespace (`math.racine(x)`).
- 41 nouveaux tests (`c/tests/test_natifs.c`, dont un aller-retour TCP
  reel contre un serveur d'echo lance par le test lui-meme, et un cycle
  fichier complet ecriture/lecture/ajout/suppression) : 122 au total.

## [1.0.4] - 2026-09-24

### Ajoute
- Systeme de modules en C (`importe "fichier.elg" comme alias`), avec
  acces aux fonctions/variables exportees via `module.nom` et
  `module.fonction(args)`. Chemin resolu relativement au fichier qui
  importe (imports imbriques pris en charge), alias deduit du nom de
  fichier si omis, modules mis en cache (charges une seule fois), et
  detection d'import circulaire. Reintroduit le module d'exemple
  `exemples/module_demo.elg` + `exemples/modules/mathutils.elg`,
  retires lors de la reecriture en C faute de temps.

## [1.0.3] - 2026-09-24

### Ajoute
- Refonte complete de la page de presentation (`site/index.html`) :
  identite visuelle premium (typographie, degrade de marque, barre de
  statistiques), documentation complete alignee sur la syntaxe actuelle,
  section licence et politique de confidentialite, section creation
  attribuee a Abiye Enzo.

## [1.0.1] - [1.0.2]

### Ajoute
- `demande` accepte desormais une syntaxe avec parentheses,
  `demande(nom, "invite")`, en plus de l'ancienne forme `demande nom
  "invite"` (toujours valide).
- Virgule optionnelle comme separateur dans les litteraux de liste, les
  arguments d'appel de fonction et les parametres de definition de
  fonction (`[1, 2, 3]`, `fonction f(a, b):`, `f(1, 2)`).
- Logo du langage (`packaging/icone.ico`), embarque dans les trois `.exe`
  via une ressource Windows par executable (`c/ressources_*.rc`) :
  visible dans l'Explorateur, la barre des taches et la barre de titre
  de l'editeur. Script `packaging/associer_fichiers.ps1` (+ wrapper
  `.bat` a double-cliquer) pour associer l'extension `.elg` a ce logo et
  a l'editeur, sans droit administrateur. Logo egalement ajoute en
  favicon et en-tete de `site/index.html`.
- Metadonnees d'auteur dans les trois `.exe` (onglet Details des
  proprietes du fichier dans l'Explorateur) : societe "AE Corporation",
  copyright "Abiye Enzo", nom/version du produit.
- Fichier `LICENSE` : tous droits reserves (Abiye Enzo / AE Corporation).
- Installateur Windows (`packaging/installateur.iss`, compile en CI avec
  Inno Setup) : `EasyLanguage-installateur.exe` installe les trois
  executables dans le profil utilisateur (aucun droit admin requis),
  cree les raccourcis Bureau/menu Demarrer, associe `.elg` au logo et a
  l'editeur, et propose d'ajouter Easy Language au `PATH` utilisateur
  pour l'utiliser depuis `cmd`/PowerShell. Publie en release a cote du
  zip portable existant.

### Corrige
- Editeur graphique : le champ d'entree standard etait un buffer a
  pre-remplir avant de lancer le script (F5), ferme des le lancement.
  Si une valeur manquait, `demande` lisait une entree vide, causant des
  variables vides puis des erreurs de type sur les comparaisons
  (`l'operateur '>=' attend des nombres`). Le champ d'entree reste
  maintenant ouvert pendant toute l'execution (comme pour le
  debogueur) : l'utilisateur tape sa reponse au moment ou `demande`
  l'attend reellement, puis Entree.
- Editeur graphique : la coloration syntaxique ne colorait correctement
  que les mots-cles proches du debut du fichier. `GetWindowTextW`
  renvoie les sauts de ligne en `\r\n` (2 caracteres) alors que
  `EM_SETSEL`/`EM_SETCHARFORMAT` comptent chaque saut de paragraphe
  comme un seul caractere (`\r`) : les positions calculees derivaient
  d'un caractere par ligne, decalant de plus en plus la coloration des
  chaines, nombres et commentaires au fil du fichier. Le tampon est
  desormais compacte (les `\n` retires) avant de calculer les positions.

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
