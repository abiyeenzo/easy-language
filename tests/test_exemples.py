import unittest
from unittest.mock import patch

from tests.utils import chemin_exemple, executer_fichier


class TestExemples(unittest.TestCase):
    """Garde-fou de non-regression: les scripts dans exemples/ doivent toujours s'executer sans erreur."""

    def test_bonjour(self):
        sortie = executer_fichier(chemin_exemple("bonjour.elg"))
        self.assertIn("Bonjour monde", sortie)
        self.assertIn("tour numero 3", sortie)

    def test_fibonacci(self):
        sortie = executer_fichier(chemin_exemple("fibonacci.elg"))
        self.assertIn("fibonacci de 10 = 55", sortie)

    def test_listes(self):
        sortie = executer_fichier(chemin_exemple("listes.elg"))
        self.assertIn("somme: 15", sortie)

    def test_module_demo(self):
        sortie = executer_fichier(chemin_exemple("module_demo.elg"))
        self.assertIn("carres: [1 4 9 16 25]", sortie)

    def test_entree(self):
        with patch("builtins.input", side_effect=["Alice", "25"]):
            sortie = executer_fichier(chemin_exemple("entree.elg"))
        self.assertIn("tu es majeur", sortie)


if __name__ == "__main__":
    unittest.main()
