# Guide de l'utilisateur Easy Language

Bienvenue ! Ce guide explique comment utiliser Easy Language sans avoir
besoin de connaissances techniques particulieres.

## C'est quoi, Easy Language ?

Un langage de programmation simple, avec des mots-cles en francais
(`soit`, `si`, `pour`, `affiche`...). Les fichiers que vous ecrivez
portent l'extension `.elg`.

## Apres l'installation

Le raccourci **Easy Language - Editeur** (Bureau ou menu Demarrer) ouvre
l'editeur. C'est par la qu'on commence.

## Ecrire et lancer votre premier script

1. Ouvrez l'editeur.
2. Ecrivez par exemple :

   ```
   soit nom = "monde"
   affiche "Bonjour" nom
   ```

3. Appuyez sur **F5** (ou menu *Executer > Lancer*). Le resultat s'affiche
   dans la zone "Sortie" en bas de la fenetre.
4. Si votre script utilise `demande` (pour lire une reponse de
   l'utilisateur), ecrivez les valeurs a l'avance dans la zone
   "Entree standard", une par ligne.

## Trouver une erreur (le debogueur)

Appuyez sur **F6** au lieu de F5. Le programme s'arrete a la premiere
ligne. Ecrivez une commande dans le champ "Commande debogueur" en bas,
puis Entree :

| Commande | Effet |
|---|---|
| `n` | executer la ligne et s'arreter a la suivante |
| `c` | continuer jusqu'au prochain point d'arret |
| `v` | afficher toutes les variables visibles a cet instant |
| `p x` | afficher la valeur de la variable `x` |
| `ba 12` | ajouter un point d'arret a la ligne 12 |
| `q` | arreter l'execution |

## Aide-memoire de syntaxe

```
soit x = 5                    variable
x = x + 1                     reaffectation (sans 'soit')
affiche "valeur:" x            afficher plusieurs valeurs
demande age "Ton age : "       lire une entree

si x > 10 alors:
    affiche "grand"
sinon:
    affiche "petit"

tantque x > 0 alors:
    x = x - 1

pour i de 1 jusqua 5 alors:
    affiche i

fonction carre(x):
    retourne x * x

soit fruits = ["pomme" "banane"]
affiche fruits[0]
pour chaque f dans fruits alors:
    affiche f
```

Pas de virgule, pas de point-virgule, pas d'accolade : les blocs sont
delimites par l'indentation, apres `:`, comme en Python.

## Des exemples tout prets

Le dossier `exemples` (installe avec le programme) contient des scripts
qui fonctionnent deja : ouvrez-les depuis l'editeur pour vous en inspirer.

## Verifier votre version

Menu **Aide > A propos** dans l'editeur, ou en ligne de commande :
`easy_language.exe --version`. La liste des changements par version est
dans `CHANGELOG.md`.

## Besoin d'aide ?

Code source, documentation complete et nouvelles versions :
https://github.com/abiyeenzo/easy-language
