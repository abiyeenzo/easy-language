# Easy Language

Un langage de programmation interprété en français, avec des fichiers `.elg`.
Interpréteur écrit en C pur (`c/`, aucune dépendance externe), avec des
points chauds en assembleur x86-64 inline pour l'arithmétique entière.

Ce README est pour les développeurs. Pour un guide simple destiné aux
utilisateurs de l'éditeur/interpréteur installé, voir
[`GUIDE_UTILISATEUR.md`](GUIDE_UTILISATEUR.md). Historique des versions :
[`CHANGELOG.md`](CHANGELOG.md).

Pas de point-virgule, pas de virgule, pas d'accolade : les blocs sont
délimités par indentation (comme en Python), après un `:`.

## Compilation

```
cd c
make linux     # easy_language, easy_debogueur (pour developper/tester ici)
make windows   # + easy_editeur.exe, via cross-compilation mingw-w64
```

Voir [`packaging/README.md`](packaging/README.md) pour la CI et le detail
de la cross-compilation.

## Tests

```
cd c
make tests
```

Suite de tests maison (`c/tests/`, aucune dependance externe) : pilote le
binaire `easy_language`/`easy_debogueur` compile comme boite noire (le
plus simple pour couvrir aussi les cas d'erreur, qui font `exit(1)`).
158 verifications : variables, boucles, fonctions, listes, erreurs, entree
standard, debogueur, bibliotheque standard (math/os/reseau/gui/texte/temps,
y compris un aller-retour TCP reel contre un serveur d'echo lance par le
test lui-meme).

## Utilisation

```
c/easy_language exemples/bonjour.elg
c/easy_language --version
c/easy_language --aide              # usage, ou lancé sans argument
c/easy_language --aide modules      # modules de la bibliotheque standard disponibles
```

## Editeur et debogueur

```
c/easy_debogueur programme.elg [lignes_arret...]   # debogueur en ligne de commande
```
Commandes du debogueur : `n` (suivant), `c` (continuer), `ba <ligne>` /
`br <ligne>` (ajouter/retirer un point d'arret), `v` (variables),
`p <nom>` (une variable), `q` (quitter).

L'éditeur graphique (`c/easy_editeur.exe`) est écrit en Win32 C pur
(RichEdit pour la coloration syntaxique, `CreateProcess`/pipes pour
lancer `easy_language.exe`/`easy_debogueur.exe`) : F5 = lancer, F6 =
déboguer, Ctrl+F = rechercher. Une barre d'outils (Nouveau, Ouvrir,
Enregistrer, Lancer, Déboguer, Rechercher) et une barre d'état (état de
l'exécution en cours, ligne/colonne du curseur) complètent le menu. Il
vérifie automatiquement (via l'API GitHub Releases, en tâche de fond, au
démarrage) si une nouvelle version est disponible, et propose d'ouvrir la
page de téléchargement le cas échéant (menu Aide > Vérifier les mises à
jour pour relancer la vérification manuellement) ; aucun téléchargement
ni exécution automatique, l'utilisateur choisit toujours. Il ne se
compile que pour Windows et doit rester dans le même dossier que les
deux autres `.exe`.

Les trois `.exe` embarquent le logo du langage (`packaging/icone.ico`,
compilé via `c/ressources.rc`). Le script `packaging/associer_fichiers.ps1`
(ou son wrapper `.bat`, à double-cliquer) associe l'extension `.elg` à ce
logo et à l'éditeur pour l'utilisateur courant, sans droit administrateur.

## Site

`site/index.html` est le site de presentation + telechargement (liens
vers `github.com/abiyeenzo/easy-language`). Voir
`.github/workflows/pages.yml` pour le publier automatiquement via GitHub
Pages une fois le depot pousse.

## Syntaxe

### Variables

```
soit x = 5
soit nom = "Alice"
x = x + 1          # réaffectation (sans 'soit')
```

### Affichage et entrée

```
affiche "Bonjour" nom "tu as" age "ans"
demande(age, "Ton age : ")
```
`demande` convertit automatiquement en nombre si possible, sinon garde du texte.
La forme sans parentheses (`demande age "Ton age : "`) reste acceptee.

### Conditions

```
si age >= 18 alors:
    affiche "majeur"
sinon si age >= 13 alors:
    affiche "ado"
sinon:
    affiche "enfant"
```

### Boucles

```
tantque i <= 5 alors:
    affiche i
    i = i + 1

pour i de 1 jusqua 10 alors:
    affiche i

pour i de 10 jusqua 0 pas -2 alors:
    affiche i
```
`arrete` sort de la boucle, `continue` passe au tour suivant.

### Fonctions

```
fonction carre(x):
    retourne x * x

affiche carre(5)
```
Les arguments multiples sont séparés par des espaces, pas des virgules :
`addition(1 2 3)`.

### Listes / tableaux

Pas de virgule : les éléments sont séparés par des espaces, entre crochets.

```
soit fruits = ["pomme", "banane", "cerise"]
affiche fruits[0]
fruits[1] = "mangue"

ajoute(fruits "kiwi")
soit x = retire(fruits 0)      # retire et retourne l'element a cet index
affiche contient(fruits "kiwi")

pour chaque fruit dans fruits alors:
    affiche fruit

soit matrice = [[1, 2], [3, 4]]
affiche matrice[1][0]
```

Une liste littérale ne peut pas être indexée directement (`[1 2][0]` est
invalide, à cause de l'ambiguïté avec une liste de listes adjacente sans
virgule) : passez par une variable d'abord.

### Types et opérateurs

- Nombres : `5`, `3.14`
- Texte : `"bonjour"`
- Booléens : `vrai`, `faux`
- Rien : `rien`
- Listes : `[1, 2, 3]` (la virgule est optionnelle : `[1 2 3]` marche aussi)
- Arithmétique : `+ - * / %` (le `+` concatène aussi textes et listes)
- Comparaison : `== != < > <= >=`
- Logique : `et`, `ou`, `non`
- Fonctions natives : `longueur(x)`, `nombre(x)`, `entier(x)`, `texte(x)`,
  `ajoute(liste valeur)`, `retire(liste index)`, `contient(liste valeur)`

### Modules

Chaque fichier `.elg` peut être importé comme module, avec ses fonctions
et variables accessibles via `module.nom`. Le chemin est relatif au
fichier qui importe (les imports imbriqués fonctionnent).

```
importe "modules/mathutils.elg" comme math

affiche math.PI
affiche math.carre(5)
```

Sans `comme alias`, le nom du module est déduit du nom de fichier
(`"mathutils.elg"` → `mathutils`). Un module n'est chargé qu'une seule
fois même s'il est importé plusieurs fois (mis en cache par chemin), et
un import circulaire est détecté et signalé comme erreur plutôt que de
boucler indéfiniment.

### Bibliothèque standard

Le dossier [`bibliotheque/`](bibliotheque/) contient un petit équivalent
des modules `math`, `os` et `socket` de Python (plus des boîtes de
dialogue simples), sous forme de fichiers `.elg` important les fonctions
natives correspondantes (implémentées en C dans `c/natifs_*.c`).

Ces modules-là s'importent **sans chemin**, par leur simple nom (comme
`import math` en Python) :

```
importe math
importe os
importe reseau
importe gui
importe texte
importe temps

affiche math.racine(2)
affiche os.fichier_existe("notes.txt")
soit s = reseau.connecter("example.com" 80)
gui.message("Titre" "Un message")
affiche texte.majuscules("bonjour")
affiche temps.formater(temps.maintenant() "%Y-%m-%d")
```

`importe <nom>` (identifiant nu, sans guillemets) est résolu contre le
dossier `bibliotheque/` fourni à côté de l'exécutable, quel que soit le
dossier depuis lequel le script est lancé (contrairement à `importe
"chemin.elg"`, résolu relativement au script). Un alias reste possible
(`importe math comme m`). Importer un nom qui n'existe pas dans la
bibliothèque standard donne une erreur claire listant les modules
disponibles ; la liste est aussi accessible via `easy_language --aide
modules` (voir [Utilisation](#utilisation)).

Ce mécanisme est réservé aux six modules ci-dessous. Pour importer votre
propre fichier `.elg` (les vôtres, ou ceux de quelqu'un d'autre), la
syntaxe historique par chemin continue de fonctionner exactement comme
avant (voir [Modules](#modules) plus haut) : `importe "chemin.elg"`.

- **`math`** : `racine`, `puissance`, `sin`, `cos`, `tan`, `abs`,
  `plancher`, `plafond`, `arrondi`, `log`, `exp`, `alea`, `alea_entier`,
  `min`, `max`, constantes `PI` et `E`.
- **`os`** : `fichier_existe`, `lire_fichier`, `ecrire_fichier`,
  `ajouter_fichier`, `supprimer_fichier`, `repertoire_courant`,
  `variable_environnement`, `horodatage`, `dormir`. `lire_fichier` sur un
  fichier absent retourne `rien` (pas une erreur : c'est un cas normal
  pour ce type d'opération, à la différence des erreurs de programme qui
  arrêtent le script).
- **`reseau`** : client TCP basique (`connecter`, `envoyer`, `recevoir`,
  `fermer`). Pas de serveur, pas de HTTP/TLS : juste un socket brut.
- **`gui`** : `message(titre texte)` et `question(titre texte)`, deux
  boîtes de dialogue simples (pas un framework de fenêtres). Sous Windows
  (`.exe`), une vraie boîte Win32 s'affiche (`MessageBoxW`) ; ailleurs
  (développement, tests, CI Linux), repli console pour rester exécutable
  et testable partout.
- **`texte`** : `decouper` (split), `joindre` (join), `remplacer`,
  `majuscules`, `minuscules`, `rogner` (trim), `commence_par`,
  `finit_par`, `sous_texte` (substring, indices bornés sans erreur),
  `inverse`, `position` (index ou `-1`), `contient_texte`.
- **`temps`** : `maintenant` (horodatage courant), `annee`, `mois`,
  `jour`, `heure`, `minute`, `seconde`, `jour_semaine` (0 = dimanche),
  `formater(horodatage motif)` avec les codes `strftime` standards
  (`%Y-%m-%d %H:%M:%S`, etc.). Toutes ces fonctions décomposent un
  horodatage (le même type que `os.horodatage()`) dans le fuseau horaire
  local de la machine.

Ces fonctions natives sont aussi appelables directement sans import
(`affiche racine(4)`), le module ne fait qu'ajouter un espace de noms.

### Commentaires

```
# ceci est un commentaire
```

## Limitations connues

- Comme il n'y a pas de virgule, un argument multi-mots contenant un `+`/`-`
  en tête peut être avalé par l'expression précédente (ex: `affiche 3 -4`
  est lu comme `3 - 4`, pas deux arguments). Utilisez des parenthèses pour
  lever l'ambiguïté : `affiche 3 (-4)`.
- **Pas de fermetures (closures) au-dela de la duree de vie d'un bloc.**
  La version C libere la memoire d'une portee (bloc `si`/boucle/appel de
  fonction) des qu'elle se termine, pour rester rapide sans ramasse-miettes.
  Definir une fonction a l'interieur d'un bloc puis l'appeler apres coup
  n'est donc pas garanti fonctionner ; definissez les fonctions au niveau
  superieur du fichier.
- L'éditeur graphique (Win32) n'a pas pu être testé visuellement dans cet
  environnement de developpement (pas de Windows/Wine disponible) ; sa
  compilation croisee reussit sans erreur, mais un test manuel sur une
  vraie machine Windows est recommande.

## Performance

**Face a notre propre tout premier prototype** (le tres original
interpreteur ecrit en Python, avant la reecriture complete en C) : sur
`fib(30)` recursif, **~30x plus rapide** en temps reel (2min26s -> 4.9s),
jusqu'a ~38x en temps CPU pur. Le gain vient surtout de la compilation
native (pas d'interpretation bytecode, pas de typage dynamique par objet
Python) ; les quelques routines en assembleur inline sur l'arithmetique
entiere (`+ - * -unaire`) ne font que documenter une integration ASM
reelle sur le chemin le plus emprunte, un `-O2` C fait deja aussi bien
sur des operations aussi simples.

**Face a du vrai CPython** (3.13), sur `fib(30)` recursif (mesures sur
cette machine de developpement, a prendre comme un ordre de grandeur) :

| Version | fib(30) | Boucle simple (20M iterations) |
|---|---|---|
| Avant optimisation | ~1.0-1.2s | ~2.5s |
| Apres reutilisation des environnements | ~0.7s | ~1.8s |
| Apres cache de resolution (appels + variables) | **~0.22s** | **~1.5s** |
| CPython 3.13 (reference) | ~0.09s | ~1.4s |

Sur la boucle simple, on est desormais dans le bruit de mesure par
rapport a CPython. Sur `fib(30)`, l'ecart est tombe d'environ 8-10x a
environ 2.5x. Honnetement : toujours plus lent que du vrai CPython sur
de la recursion intensive, mais l'ecart s'est nettement resserre, sans
rien casser (158 tests toujours au vert a chaque etape).

Deux optimisations ont fait le gros du travail :

- **Reutilisation des environnements.** Chaque appel de fonction ou
  entree de bloc (`si`, boucle) allouait et liberait un nouvel
  environnement (jusqu'a 4 `malloc`/`free` par appel), un cout qui
  dominait le temps d'execution sur du code recursif (visible en temps
  systeme). Les environnements sont maintenant recycles via une pile de
  reutilisation, et les noms de variables ne sont plus dupliques (ils
  vivent deja aussi longtemps que le programme, dans l'arbre syntaxique).
- **Cache de resolution sur les noeuds de l'arbre syntaxique.** Chaque
  appel de fonction verifiait d'abord si le nom appelait une fonction
  native, en parcourant sequentiellement toutes les tables de fonctions
  natives (jusqu'a une soixantaine de comparaisons de chaines) avant de
  chercher une fonction utilisateur : sur une fonction recursive comme
  `fib`, ce parcours se repetait des millions de fois pour le meme site
  d'appel, en pure perte. De meme, chaque acces a une variable remontait
  la chaine des portees parentes avec une comparaison de chaines a
  chaque niveau. Comme ces deux resolutions ne dependent que du code
  (jamais de l'etat d'execution), leur resultat est maintenant mis en
  cache directement sur le noeud d'AST concerne des la premiere
  execution, et reutilise ensuite sans reparcourir quoi que ce soit.
  Le cache est invalide-safe : toute incoherence retombe automatiquement
  sur la recherche complete plutot que de retourner un mauvais resultat.

La prochaine vraie piste pour aller plus loin serait de resoudre les
variables a des emplacements fixes au moment du parsing plutot qu'a la
premiere execution, ou de passer a un modele par bytecode plutot que
tree-walking : un chantier plus consequent que les deux ci-dessus.

## Distribution sous Windows

Aucune dépendance à installer sur la machine cible : les trois `.exe`
(`easy_language.exe`, `easy_debogueur.exe`, `easy_editeur.exe`) ne
dépendent que de DLL systeme presentes par defaut sur Windows
(`kernel32`, `msvcrt`, `user32`, `gdi32`, `comdlg32`, `comctl32` pour la
barre d'outils/d'état de l'éditeur, `ws2_32` pour le reseau). Voir
[`packaging/README.md`](packaging/README.md) pour la compilation, et
[`packaging/installateur.iss`](packaging/installateur.iss) pour
l'installateur (menu Demarrer, association `.elg`, ajout au `PATH`).

## Licence

Tous droits reserves © 2026 Abiye Enzo / AE Corporation. Voir
[`LICENSE`](LICENSE).
