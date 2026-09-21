import os
import sys

from easy_language.lexer import Lexer
from easy_language.analyseur import Analyseur
from easy_language.interpreteur import Interpreteur
from easy_language.erreurs import ErreurEasyLang


def executer_fichier(chemin):
    if not chemin.endswith(".elg"):
        print(f"Attention: '{chemin}' n'a pas l'extension .elg", file=sys.stderr)

    try:
        with open(chemin, "r", encoding="utf-8") as f:
            source = f.read()
    except OSError as e:
        print(f"Erreur: impossible de lire le fichier '{chemin}' ({e})", file=sys.stderr)
        sys.exit(1)

    try:
        jetons = Lexer(source).tokeniser()
        programme = Analyseur(jetons).analyser()
        dossier = os.path.dirname(os.path.abspath(chemin))
        Interpreteur().executer(programme, dossier)
    except ErreurEasyLang as e:
        if e.ligne is not None:
            print(f"Erreur ligne {e.ligne}: {e.message}", file=sys.stderr)
        else:
            print(f"Erreur: {e.message}", file=sys.stderr)
        sys.exit(1)


def main():
    if len(sys.argv) != 2:
        print("Usage: python main.py <fichier.elg>", file=sys.stderr)
        sys.exit(1)
    executer_fichier(sys.argv[1])


if __name__ == "__main__":
    main()
