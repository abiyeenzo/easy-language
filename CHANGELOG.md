# Changelog

Toutes les versions notables du projet sont documentees ici.

## [1.0.7] - 2026-09-24

### Ajoute
- **Module `gui` : de vraies fenetres Win32.** En plus des boites de
  dialogue existantes (`message`/`question`), le module expose
  desormais `creer`, `bouton`, `etiquette`, `champ_texte`, `lire_champ`,
  `definir_champ`, `sur_clic` et `executer` pour construire de vraies
  fenetres Win32 avec de vrais composants et de vrais evenements : un
  clic sur un bouton peut rappeler directement une fonction Easy
  Language (`interpreteur_appeler`, nouvelle API publique de
  l'interpreteur pour les rappels depuis les natifs). Disponible
  uniquement sous Windows (`.exe`) ; erreur claire ailleurs. Exemple
  complet dans `bibliotheque/gui.elg`.
- **Editeur graphique, refonte de l'interface :**
  - Barre d'outils retiree (elle faisait une deuxieme ligne redondante
    avec le menu, qui couvre deja toutes ses actions) : une seule barre
    en haut desormais.
  - Numeros de ligne dans une gouttiere a gauche de l'editeur (retour a
    la ligne automatique desactive pour garder une ligne logique = une
    ligne visuelle, necessaire pour que les numeros restent alignes).
  - Plusieurs fichiers ouverts a la fois, en onglets (jusqu'a 20) :
    Nouveau/Ouvrir ajoutent un onglet, un fichier deja ouvert bascule
    sur son onglet au lieu d'un doublon, `*` signale les modifications
    non enregistrees, confirmation a la fermeture (Ctrl+W) si besoin.
  - Barre laterale avec une arborescence basique (dossier du fichier
    ouvert, navigation par dossier, clic sur un `.elg` pour l'ouvrir).
  - Menu Fichier > Fichiers recents (jusqu'a 8, persistes par
    utilisateur dans le registre Windows HKCU), avec option pour vider
    la liste.
- **Fermetures (closures).** Une fonction definie a l'interieur d'un bloc
  (`si`/boucle/appel de fonction) peut desormais etre retournee ou
  stockee ailleurs et rester appelable apres la fin de ce bloc, en
  gardant ses variables capturees (y compris plusieurs fermetures
  independantes issues du meme point du code). Auparavant documente
  comme une limitation connue. Techniquement : `Environnement` compte
  maintenant ses references (une par portee enfant, une par fonction
  qui l'a capture via `environnement_capturer`) et n'est recycle que
  quand il n'est plus reference, au lieu d'etre detruit
  inconditionnellement a la fin du bloc. Cout mesure : ~15-20% plus lent
  sur de la recursion tres intensive (`fib(30)`), negligeable ailleurs.
  4 nouveaux tests de non-regression.
- **Ambiguite `3 -4` levee.** Dans une liste d'arguments sans virgule
  (`affiche`, appels, listes litterales), un `-` precede d'une espace
  mais colle a l'operande suivant marque desormais le debut d'un nouvel
  argument plutot qu'une continuation de l'expression courante (meme
  principe que Ruby pour le meme probleme) : `affiche 3 -4` affiche deux
  arguments au lieu de calculer `3 - 4`. `affiche 3 - 4` et
  `affiche 3-4` restent des soustractions, inchangees. Auparavant
  documente comme une limitation connue (contournement par parentheses).
  Le lexer garde maintenant, par jeton, si une espace le precedait
  immediatement. 5 nouveaux tests de non-regression.
- Installateur : page dediee (auteur, societe, depot) affichee avant
  l'installation (`InfoBeforeFile`).
- Site : bouton "Copier la doc (pour un LLM)" sur la section
  Documentation, qui construit un Markdown propre a partir de la doc
  affichee (sans copie separee a maintenir) et le met dans le
  presse-papier.
- 12 nouveaux tests (190 au total).

### Corrige
- La section "Limitations connues" du site (accordéon de documentation)
  mentionnait encore l'absence de fermetures, deja corrigee plus haut
  dans cette meme version : trouve en testant le nouveau bouton "Copier
  la doc" (qui extrait le texte reellement affiche), mis a jour pour
  refleter l'etat actuel.

## [1.0.6] - 2026-09-24

### Change
- L'interpreteur en ligne de commande est renomme `easy_language`(`.exe`)
  -> `easylang`(`.exe`), pour une commande plus courte a taper une fois
  installe et ajoute au `PATH`. Seul l'interpreteur est renomme ; le
  debogueur (`easy_debogueur`) et l'editeur (`easy_editeur`) gardent
  leur nom. Toutes les references (Makefile, CI, installateur, README,
  guide utilisateur, site) sont mises a jour ; une installation
  existante d'une version anterieure devra etre remplacee (l'installateur
  desinstalle proprement l'ancienne version avant d'installer la
  nouvelle).

### Ajoute
- Module standard `texte` (`bibliotheque/texte.elg`, natif
  `c/natifs_texte.c`) : decouper (split), joindre (join), remplacer,
  majuscules/minuscules, rogner (trim), commence_par/finit_par,
  sous_texte (substring a indices bornes), inverse, position,
  contient_texte.
- Module standard `temps` (`bibliotheque/temps.elg`, natif
  `c/natifs_temps.c`) : maintenant, annee/mois/jour/heure/minute/seconde,
  jour_semaine, formater(horodatage motif) via les codes strftime
  standards.
- Editeur graphique : barre d'outils (Nouveau, Ouvrir, Enregistrer,
  Lancer, Deboguer, Rechercher), barre d'etat (etat de l'execution en
  cours, position ligne/colonne du curseur), recherche de texte
  (Ctrl+F, boite de dialogue standard Windows).
- Editeur graphique : verification automatique des mises a jour (API
  GitHub Releases, en tache de fond au demarrage, sans bloquer
  l'interface). Si une version plus recente existe, propose d'ouvrir la
  page de telechargement (aucun telechargement ni execution automatique,
  toujours a la demande de l'utilisateur). Menu Aide > "Verifier les
  mises a jour" pour relancer la verification manuellement.
- `importe <nom>` (identifiant nu, sans guillemets) pour la bibliotheque
  standard : `importe math`, `importe os`, `importe reseau`, `importe
  gui`, `importe texte`, `importe temps`, avec ou sans alias (`importe
  math comme m`). Resolu contre le dossier `bibliotheque/` fourni a cote
  de l'executable, quel que soit le dossier d'ou le script est lance
  (avec repli sur `bibliotheque/` un niveau au-dessus pour la mise en
  page du depot en developpement). La forme historique par chemin
  (`importe "chemin.elg"`) continue de fonctionner a l'identique, pour
  les fichiers de l'utilisateur.
- `easylang` (ligne de commande) : lance sans argument, affiche
  desormais un ecran d'aide au lieu d'une erreur ; `--aide`/`--help`/
  `-h` pour l'usage, `--aide modules` (ou `--modules`) pour lister les
  modules de la bibliotheque standard disponibles avec une courte
  description de chacun.
- 40 nouveaux tests (`c/tests/test_texte_temps.c`,
  `c/tests/test_import_systeme.c`) : 178 au total.

### Ameliore
- Reutilisation des `Environnement` (bloc/boucle/appel de fonction) via
  une pile de recyclage au lieu d'un `malloc`/`free` a chaque fois, et
  arret de la duplication des noms de variables (ils vivent deja aussi
  longtemps que le programme, dans l'arbre syntaxique).
- Cache de resolution sur les noeuds de l'arbre syntaxique, pour deux
  chemins chauds de l'evaluateur :
  - Chaque appel de fonction verifiait sequentiellement toutes les
    tables de fonctions natives (jusqu'a ~60 comparaisons de chaines)
    avant de chercher une fonction utilisateur, a chaque appel. Cette
    appartenance "natif ou non" ne depend que du texte du nom, jamais de
    l'etat d'execution : elle est maintenant resolue une seule fois par
    site d'appel puis mise en cache.
  - Chaque acces/assignation de variable remontait la chaine des
    portees parentes avec une comparaison de chaines a chaque niveau.
    La position d'une variable pour un noeud d'AST donne est egalement
    stable d'une execution a l'autre : elle est maintenant mise en
    cache (avec verification et repli automatique sur la recherche
    complete en cas d'incoherence, jamais de resultat incorrect).
  - Ensemble, sur `fib(30)` recursif : ~1.0-1.2s -> ~0.22s. L'ecart avec
    CPython 3.13 sur ce test tombe d'environ 8-10x a environ 2.5x. Sur
    une boucle simple sans appel de fonction, l'ecart devient
    negligeable. Comportement inchange (158 tests toujours au vert, plus
    un test de non-regression cible sur la recursion/reaffectation/
    imbrication de portees).
- README : tableau de mesures Performance mis a jour avec les trois
  etapes (avant optimisation, apres reutilisation des environnements,
  apres cache de resolution) face a CPython 3.13.

### Corrige
- **Bug introduit plus tot dans cette meme version** (reutilisation des
  environnements, ci-dessus) : `importe "chemin.elg"` sans `comme alias`
  calculait l'alias par defaut dans un tampon de pile local, or
  `environnement_definir` emprunte desormais le pointeur du nom au lieu
  de le dupliquer. Un deuxieme `importe` sans alias reutilisait cette
  meme pile et corrompait silencieusement le nom du premier module deja
  lie (pointeur pendouillant), rendant son acces (`module.quelquechose`)
  impossible ensuite. Corrige en allouant cet alias par defaut sur le
  tas ; couvert par un test de non-regression dedie
  (`test_import_systeme_deux_imports_sans_alias`).
- La section Performance du README comparait uniquement a notre tout
  premier prototype (ecrit en Python), ce qui laissait croire a une
  victoire generale contre Python. Chiffres ajoutes face a du vrai
  CPython 3.13 : plus lent sur `fib(30)` recursif, proche sur une boucle
  simple.

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
