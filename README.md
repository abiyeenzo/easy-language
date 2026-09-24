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
l'exécution en cours, ligne/colonne du curseur) complètent le menu. Il ne
se compile que pour Windows et doit rester dans le même dossier que les
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
natives correspondantes (implémentées en C dans `c/natifs_*.c`) :

```
importe "bibliotheque/math.elg" comme math
importe "bibliotheque/os.elg" comme os
importe "bibliotheque/reseau.elg" comme reseau
importe "bibliotheque/gui.elg" comme gui
importe "bibliotheque/texte.elg" comme texte
importe "bibliotheque/temps.elg" comme temps

affiche math.racine(2)
affiche os.fichier_existe("notes.txt")
soit s = reseau.connecter("example.com" 80)
gui.message("Titre" "Un message")
affiche texte.majuscules("bonjour")
affiche temps.formater(temps.maintenant() "%Y-%m-%d")
```

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

Comparaison sur `fib(30)` recursif (interpreteur precedent en Python vs
celui-ci en C) : **~30x plus rapide** en temps reel (2min26s -> 4.9s),
jusqu'a ~38x en temps CPU pur. Le gain vient surtout de la compilation
native (pas d'interpretation bytecode, pas de typage dynamique par objet
Python) ; les quelques routines en assembleur inline sur l'arithmetique
entiere (`+ - * -unaire`) ne font que documenter une integration ASM
reelle sur le chemin le plus emprunte, un `-O2` C fait deja aussi bien
sur des operations aussi simples.

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
