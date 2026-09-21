import contextlib
import io
import os

from easy_language.analyseur import Analyseur
from easy_language.interpreteur import Interpreteur
from easy_language.lexer import Lexer


def analyser_source(source):
    jetons = Lexer(source).tokeniser()
    return Analyseur(jetons).analyser()


def executer(source, dossier=None):
    """Execute du code source Easy Language et retourne ce qui a ete affiche."""
    programme = analyser_source(source)
    interpreteur = Interpreteur()
    tampon = io.StringIO()
    with contextlib.redirect_stdout(tampon):
        interpreteur.executer(programme, dossier)
    return tampon.getvalue()


def executer_fichier(chemin):
    with open(chemin, "r", encoding="utf-8") as f:
        source = f.read()
    dossier = os.path.dirname(os.path.abspath(chemin))
    return executer(source, dossier)


RACINE_PROJET = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def chemin_exemple(*segments):
    return os.path.join(RACINE_PROJET, "exemples", *segments)
