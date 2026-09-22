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
81 verifications : variables, boucles, fonctions, listes, erreurs, entree
standard, debogueur.

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
déboguer. Il ne se compile que pour Windows et doit rester dans le même
dossier que les deux autres `.exe`.

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

### Commentaires

```
# ceci est un commentaire
```

## Limitations connues

- Comme il n'y a pas de virgule, un argument multi-mots contenant un `+`/`-`
  en tête peut être avalé par l'expression précédente (ex: `affiche 3 -4`
  est lu comme `3 - 4`, pas deux arguments). Utilisez des parenthèses pour
  lever l'ambiguïté : `affiche 3 (-4)`.
- **Modules (`importe`) non supportés dans la version C** (ils existaient
  dans une version antérieure en Python, retirée). Portage possible plus
  tard si besoin.
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
(`kernel32`, `msvcrt`, `user32`, `gdi32`, `comdlg32`). Voir
[`packaging/README.md`](packaging/README.md) pour la compilation, et
[`packaging/installateur.iss`](packaging/installateur.iss) pour
l'installateur (menu Demarrer, association `.elg`, ajout au `PATH`).

## Licence

Tous droits reserves © 2026 Abiye Enzo / AE Corporation. Voir
[`LICENSE`](LICENSE).
