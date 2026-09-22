# Easy Language

Un langage de programmation interprété en français, avec des fichiers `.elg`.
Interpréteur écrit en Python pur (aucune dépendance externe) — fonctionne
à l'identique sous Linux, macOS et Windows.

Pas de point-virgule, pas de virgule, pas d'accolade : les blocs sont
délimités par indentation (comme en Python), après un `:`.

## Utilisation

```
python main.py exemples/bonjour.elg
```

## Editeur et debogueur

```
python editeur.py                    # editeur graphique (coloration syntaxique, F5 = lancer, F6 = deboguer)
python debogueur_cli.py programme.elg [lignes_arret...]   # debogueur en ligne de commande
```

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
demande age "Ton age : "
```
`demande` convertit automatiquement en nombre si possible, sinon garde du texte.

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
soit fruits = ["pomme" "banane" "cerise"]
affiche fruits[0]
fruits[1] = "mangue"

ajoute(fruits "kiwi")
soit x = retire(fruits 0)      # retire et retourne l'element a cet index
affiche contient(fruits "kiwi")

pour chaque fruit dans fruits alors:
    affiche fruit

soit matrice = [[1 2] [3 4]]
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
- Listes : `[1 2 3]`
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
(`"mathutils.elg"` → `mathutils`). Un import circulaire est détecté et
signalé comme erreur plutôt que de boucler indéfiniment.

### Commentaires

```
# ceci est un commentaire
```

## Limitation connue

Comme il n'y a pas de virgule, un argument multi-mots contenant un `+`/`-`
en tête peut être avalé par l'expression précédente (ex: `affiche 3 -4`
est lu comme `3 - 4`, pas deux arguments). Utilisez des parenthèses pour
lever l'ambiguïté : `affiche 3 (-4)`.

## Tests

Suite de tests unitaires (stdlib `unittest`, aucune dépendance à installer) :
lexer, parser, interpréteur (variables, boucles, fonctions, listes, erreurs),
système de modules, et non-régression sur les scripts d'`exemples/`.

```
python -m unittest discover -v
```

## Distribution sous Windows

Aucune dépendance : `python main.py programme.elg` fonctionne tel quel
sous Windows avec Python 3 installé. Pour un `.exe` autonome (sans exiger
Python sur la machine cible), on peut utiliser PyInstaller **depuis
Windows** :

```
pip install pyinstaller
pyinstaller --onefile --name easy_language main.py
```
