import os
import sys

from easy_language.analyseur import Analyseur
from easy_language.debogueur import ArretDebogueur, Debogueur
from easy_language.erreurs import ErreurEasyLang
from easy_language.interpreteur import Interpreteur
from easy_language.lexer import Lexer


def main():
    if len(sys.argv) < 2:
        print("Usage: python debogueur_cli.py <fichier.elg> [ligne_arret ...]", file=sys.stderr)
        sys.exit(1)

    chemin = sys.argv[1]
    points_arret = [int(x) for x in sys.argv[2:]]

    try:
        with open(chemin, "r", encoding="utf-8") as f:
            source = f.read()
    except OSError as e:
        print(f"Erreur: impossible de lire le fichier '{chemin}' ({e})", file=sys.stderr)
        sys.exit(1)

    lignes_source = source.split("\n")

    try:
        jetons = Lexer(source).tokeniser()
        programme = Analyseur(jetons).analyser()
    except ErreurEasyLang as e:
        print(f"Erreur ligne {e.ligne}: {e.message}", file=sys.stderr)
        sys.exit(1)

    interpreteur = Interpreteur()
    Debogueur(interpreteur, lignes_source, points_arret)

    print("=== Debogueur Easy Language === (tapez 'aide' pour les commandes)")
    dossier = os.path.dirname(os.path.abspath(chemin))
    try:
        interpreteur.executer(programme, dossier)
        print("=== programme termine ===")
    except ArretDebogueur:
        print("=== execution arretee par l'utilisateur ===")
    except ErreurEasyLang as e:
        print(f"Erreur ligne {e.ligne}: {e.message}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
