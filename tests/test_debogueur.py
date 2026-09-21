import contextlib
import io
import unittest
from unittest.mock import patch

from easy_language.debogueur import ArretDebogueur, Debogueur
from easy_language.interpreteur import Interpreteur
from tests.utils import analyser_source


def executer_avec_debogueur(source, points_arret=None, commandes=()):
    lignes = source.split("\n")
    programme = analyser_source(source)
    interpreteur = Interpreteur()
    Debogueur(interpreteur, lignes, points_arret)
    tampon = io.StringIO()
    with patch("builtins.input", side_effect=list(commandes)):
        with contextlib.redirect_stdout(tampon):
            interpreteur.executer(programme)
    return tampon.getvalue()


class TestDebogueur(unittest.TestCase):
    def test_pas_a_pas_sans_point_arret_sarrete_a_chaque_ligne(self):
        source = 'soit x = 1\nsoit y = 2\naffiche x + y\n'
        sortie = executer_avec_debogueur(source, commandes=["n", "n", "n"])
        self.assertIn("ligne 1", sortie)
        self.assertIn("ligne 2", sortie)
        self.assertIn("ligne 3", sortie)
        self.assertIn("3\n", sortie)

    def test_point_arret_ignore_les_lignes_precedentes(self):
        source = 'soit x = 1\nsoit y = 2\naffiche x + y\n'
        sortie = executer_avec_debogueur(source, points_arret=[3], commandes=["c"])
        self.assertNotIn("ligne 1", sortie)
        self.assertNotIn("ligne 2", sortie)
        self.assertIn("ligne 3", sortie)

    def test_inspection_variable(self):
        source = 'soit x = 42\naffiche x\n'
        sortie = executer_avec_debogueur(source, points_arret=[2], commandes=["p x", "c"])
        self.assertIn("x = 42", sortie)

    def test_liste_toutes_variables(self):
        source = 'soit a = 1\nsoit b = 2\naffiche a\n'
        sortie = executer_avec_debogueur(source, points_arret=[3], commandes=["v", "c"])
        self.assertIn("a = 1", sortie)
        self.assertIn("b = 2", sortie)

    def test_quitter_leve_arret_debogueur(self):
        source = 'soit x = 1\naffiche x\n'
        with self.assertRaises(ArretDebogueur):
            executer_avec_debogueur(source, commandes=["q"])

    def test_ajout_et_retrait_point_arret_dynamique(self):
        # premier arret automatique (mode pas-a-pas) a la ligne 1: on y ajoute
        # un point d'arret a la ligne 3 puis on passe en mode 'continuer'.
        source = 'soit x = 1\nsoit y = 2\naffiche y\n'
        sortie = executer_avec_debogueur(source, commandes=["ba 3", "c", "c"])
        self.assertIn("ligne 1", sortie)
        self.assertNotIn("ligne 2", sortie)
        self.assertIn("ligne 3", sortie)


if __name__ == "__main__":
    unittest.main()
