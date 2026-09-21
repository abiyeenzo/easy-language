import unittest
from unittest.mock import patch

from easy_language.erreurs import ErreurExecution
from tests.utils import executer


class TestVariablesEtArithmetique(unittest.TestCase):
    def test_declaration_et_affichage(self):
        self.assertEqual(executer('soit x = 5\naffiche x\n'), "5\n")

    def test_reaffectation(self):
        self.assertEqual(executer('soit x = 1\nx = x + 1\naffiche x\n'), "2\n")

    def test_operations_arithmetiques(self):
        self.assertEqual(executer('affiche 2 + 3 * 4\n'), "14\n")
        self.assertEqual(executer('affiche (2 + 3) * 4\n'), "20\n")
        self.assertEqual(executer('affiche 7 % 3\n'), "1\n")
        self.assertEqual(executer('affiche 7 / 2\n'), "3.5\n")
        self.assertEqual(executer('affiche -5 + 2\n'), "-3\n")

    def test_concatenation_texte(self):
        self.assertEqual(executer('affiche "a" + "b"\n'), "ab\n")
        self.assertEqual(executer('affiche "valeur:" + 5\n'), "valeur:5\n")

    def test_repetition_texte(self):
        self.assertEqual(executer('affiche "ab" * 3\n'), "ababab\n")

    def test_division_par_zero(self):
        with self.assertRaises(ErreurExecution):
            executer('affiche 1 / 0\n')

    def test_operateur_sur_non_nombre(self):
        with self.assertRaises(ErreurExecution):
            executer('affiche "a" - 1\n')

    def test_variable_inconnue(self):
        with self.assertRaises(ErreurExecution):
            executer('affiche x\n')

    def test_affectation_sans_declaration(self):
        with self.assertRaises(ErreurExecution):
            executer('x = 1\n')


class TestAffichage(unittest.TestCase):
    def test_booleens_et_rien(self):
        self.assertEqual(executer('affiche vrai\n'), "vrai\n")
        self.assertEqual(executer('affiche faux\n'), "faux\n")
        self.assertEqual(executer('affiche rien\n'), "rien\n")

    def test_plusieurs_valeurs(self):
        self.assertEqual(executer('affiche "x =" 5 "et y =" 10\n'), "x = 5 et y = 10\n")


class TestLogique(unittest.TestCase):
    def test_comparaisons(self):
        self.assertEqual(executer('affiche 5 > 3\n'), "vrai\n")
        self.assertEqual(executer('affiche 5 == 5\n'), "vrai\n")
        self.assertEqual(executer('affiche 5 != 5\n'), "faux\n")

    def test_et_ou_non(self):
        self.assertEqual(executer('affiche vrai et faux\n'), "faux\n")
        self.assertEqual(executer('affiche vrai ou faux\n'), "vrai\n")
        self.assertEqual(executer('affiche non vrai\n'), "faux\n")

    def test_court_circuit_et(self):
        # si le court-circuit ne fonctionnait pas, l'appel a f() lancerait une erreur
        sortie = executer(
            'fonction f():\n'
            '    affiche "appelee"\n'
            '    retourne vrai\n'
            'affiche faux et f()\n'
        )
        self.assertEqual(sortie, "faux\n")

    def test_court_circuit_ou(self):
        sortie = executer(
            'fonction f():\n'
            '    affiche "appelee"\n'
            '    retourne faux\n'
            'affiche vrai ou f()\n'
        )
        self.assertEqual(sortie, "vrai\n")


class TestConditions(unittest.TestCase):
    def test_si_vrai(self):
        self.assertEqual(executer('si vrai alors:\n    affiche "oui"\n'), "oui\n")

    def test_sinon_si(self):
        sortie = executer(
            'soit x = 5\n'
            'si x > 10 alors:\n'
            '    affiche "grand"\n'
            'sinon si x > 3 alors:\n'
            '    affiche "moyen"\n'
            'sinon:\n'
            '    affiche "petit"\n'
        )
        self.assertEqual(sortie, "moyen\n")

    def test_sinon(self):
        sortie = executer('si faux alors:\n    affiche "a"\nsinon:\n    affiche "b"\n')
        self.assertEqual(sortie, "b\n")


class TestBoucles(unittest.TestCase):
    def test_tantque(self):
        sortie = executer('soit i = 0\ntantque i < 3 alors:\n    affiche i\n    i = i + 1\n')
        self.assertEqual(sortie, "0\n1\n2\n")

    def test_pour_de_jusqua(self):
        sortie = executer('pour i de 1 jusqua 3 alors:\n    affiche i\n')
        self.assertEqual(sortie, "1\n2\n3\n")

    def test_pour_avec_pas_negatif(self):
        sortie = executer('pour i de 3 jusqua 1 pas -1 alors:\n    affiche i\n')
        self.assertEqual(sortie, "3\n2\n1\n")

    def test_pour_pas_zero_interdit(self):
        with self.assertRaises(ErreurExecution):
            executer('pour i de 1 jusqua 3 pas 0 alors:\n    affiche i\n')

    def test_arrete(self):
        sortie = executer('pour i de 1 jusqua 5 alors:\n    si i == 3 alors:\n        arrete\n    affiche i\n')
        self.assertEqual(sortie, "1\n2\n")

    def test_continue(self):
        sortie = executer('pour i de 1 jusqua 4 alors:\n    si i == 2 alors:\n        continue\n    affiche i\n')
        self.assertEqual(sortie, "1\n3\n4\n")

    def test_pour_chaque_liste(self):
        sortie = executer('pour chaque x dans [10 20 30] alors:\n    affiche x\n')
        self.assertEqual(sortie, "10\n20\n30\n")

    def test_pour_chaque_texte(self):
        sortie = executer('pour chaque c dans "ab" alors:\n    affiche c\n')
        self.assertEqual(sortie, "a\nb\n")


class TestFonctions(unittest.TestCase):
    def test_fonction_simple(self):
        sortie = executer('fonction carre(x):\n    retourne x * x\naffiche carre(4)\n')
        self.assertEqual(sortie, "16\n")

    def test_fonction_recursive(self):
        sortie = executer(
            'fonction fact(n):\n'
            '    si n <= 1 alors:\n'
            '        retourne 1\n'
            '    retourne n * fact(n - 1)\n'
            'affiche fact(5)\n'
        )
        self.assertEqual(sortie, "120\n")

    def test_fonction_sans_retour_donne_rien(self):
        sortie = executer('fonction f():\n    affiche "salut"\naffiche f()\n')
        self.assertEqual(sortie, "salut\nrien\n")

    def test_mauvaise_arite(self):
        with self.assertRaises(ErreurExecution):
            executer('fonction f(a b):\n    retourne a + b\naffiche f(1)\n')

    def test_appel_non_fonction(self):
        with self.assertRaises(ErreurExecution):
            executer('soit x = 5\naffiche x(1)\n')

    def test_portee_locale(self):
        sortie = executer(
            'soit x = 1\n'
            'fonction f():\n'
            '    soit x = 2\n'
            '    affiche x\n'
            'f()\n'
            'affiche x\n'
        )
        self.assertEqual(sortie, "2\n1\n")


class TestListes(unittest.TestCase):
    def test_litterale_et_affichage(self):
        self.assertEqual(executer('affiche [1 2 3]\n'), "[1 2 3]\n")

    def test_indexation_lecture(self):
        self.assertEqual(executer('soit l = [1 2 3]\naffiche l[1]\n'), "2\n")

    def test_indexation_ecriture(self):
        sortie = executer('soit l = [1 2 3]\nl[0] = 9\naffiche l\n')
        self.assertEqual(sortie, "[9 2 3]\n")

    def test_indexation_hors_limites(self):
        with self.assertRaises(ErreurExecution):
            executer('soit l = [1]\naffiche l[5]\n')

    def test_liste_imbriquee(self):
        sortie = executer('soit m = [[1 2] [3 4]]\naffiche m[1][0]\n')
        self.assertEqual(sortie, "3\n")

    def test_ajoute_retire_contient(self):
        sortie = executer(
            'soit l = [1 2]\n'
            'ajoute(l 3)\n'
            'affiche l\n'
            'affiche retire(l 0)\n'
            'affiche l\n'
            'affiche contient(l 3)\n'
            'affiche contient(l 99)\n'
        )
        self.assertEqual(sortie, "[1 2 3]\n1\n[2 3]\nvrai\nfaux\n")

    def test_longueur(self):
        self.assertEqual(executer('affiche longueur([1 2 3 4])\n'), "4\n")

    def test_liste_vide_est_fausse(self):
        sortie = executer('si [] alors:\n    affiche "vrai"\nsinon:\n    affiche "faux"\n')
        self.assertEqual(sortie, "faux\n")

    def test_concatenation_listes(self):
        self.assertEqual(executer('affiche [1 2] + [3 4]\n'), "[1 2 3 4]\n")


class TestConversions(unittest.TestCase):
    def test_nombre(self):
        self.assertEqual(executer('affiche nombre("42")\n'), "42\n")
        self.assertEqual(executer('affiche nombre("3.5")\n'), "3.5\n")

    def test_entier(self):
        self.assertEqual(executer('affiche entier(3.9)\n'), "3\n")

    def test_texte(self):
        self.assertEqual(executer('affiche texte(5)\n'), "5\n")


class TestDemande(unittest.TestCase):
    def test_conversion_automatique_nombre(self):
        with patch("builtins.input", return_value="42"):
            sortie = executer('demande x\naffiche x + 1\n')
        self.assertEqual(sortie, "43\n")

    def test_garde_texte_si_non_numerique(self):
        with patch("builtins.input", return_value="bonjour"):
            sortie = executer('demande x\naffiche x + "!"\n')
        self.assertEqual(sortie, "bonjour!\n")


if __name__ == "__main__":
    unittest.main()
