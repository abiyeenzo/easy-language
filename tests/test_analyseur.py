import unittest

from easy_language import noeuds as n
from easy_language.erreurs import ErreurSyntaxe
from tests.utils import analyser_source


class TestAnalyseur(unittest.TestCase):
    def test_declaration(self):
        programme = analyser_source("soit x = 5")
        instruction = programme.instructions[0]
        self.assertIsInstance(instruction, n.Declaration)
        self.assertEqual(instruction.nom, "x")
        self.assertIsInstance(instruction.expression, n.Litteral)
        self.assertEqual(instruction.expression.valeur, 5)

    def test_affectation_simple(self):
        programme = analyser_source("soit x = 1\nx = 2")
        self.assertIsInstance(programme.instructions[1], n.Affectation)

    def test_si_sinon_si_sinon(self):
        source = (
            "si a alors:\n"
            "    affiche 1\n"
            "sinon si b alors:\n"
            "    affiche 2\n"
            "sinon:\n"
            "    affiche 3\n"
        )
        si = analyser_source(source).instructions[0]
        self.assertIsInstance(si, n.Si)
        self.assertEqual(len(si.branches), 2)
        self.assertIsNotNone(si.sinon)

    def test_si_sans_sinon(self):
        si = analyser_source("si a alors:\n    affiche 1\n").instructions[0]
        self.assertIsNone(si.sinon)

    def test_tantque(self):
        boucle = analyser_source("tantque vrai alors:\n    affiche 1\n").instructions[0]
        self.assertIsInstance(boucle, n.TantQue)

    def test_pour_de_jusqua_pas(self):
        boucle = analyser_source("pour i de 1 jusqua 10 pas 2 alors:\n    affiche i\n").instructions[0]
        self.assertIsInstance(boucle, n.Pour)
        self.assertEqual(boucle.variable, "i")
        self.assertIsNotNone(boucle.pas)

    def test_pour_sans_pas(self):
        boucle = analyser_source("pour i de 1 jusqua 10 alors:\n    affiche i\n").instructions[0]
        self.assertIsNone(boucle.pas)

    def test_pour_chaque(self):
        boucle = analyser_source("pour chaque x dans [1 2] alors:\n    affiche x\n").instructions[0]
        self.assertIsInstance(boucle, n.PourChaque)
        self.assertEqual(boucle.variable, "x")

    def test_fonction_et_retourne(self):
        fonction = analyser_source("fonction f(a b):\n    retourne a + b\n").instructions[0]
        self.assertIsInstance(fonction, n.DefinitionFonction)
        self.assertEqual(fonction.parametres, ["a", "b"])
        self.assertIsInstance(fonction.bloc[0], n.Retourne)

    def test_appel_fonction_arguments_sans_virgule(self):
        expr = analyser_source("f(1 2 3)").instructions[0].expression
        self.assertIsInstance(expr, n.AppelFonction)
        self.assertEqual(len(expr.arguments), 3)

    def test_liste_litterale(self):
        expr = analyser_source("soit l = [1 2 3]").instructions[0].expression
        self.assertIsInstance(expr, n.Liste)
        self.assertEqual(len(expr.elements), 3)

    def test_indexation(self):
        expr = analyser_source("soit x = l[0]").instructions[0].expression
        self.assertIsInstance(expr, n.Indexation)

    def test_affectation_index(self):
        instruction = analyser_source("l[0] = 9").instructions[0]
        self.assertIsInstance(instruction, n.AffectationIndex)

    def test_importation_avec_alias(self):
        imp = analyser_source('importe "u.elg" comme u').instructions[0]
        self.assertIsInstance(imp, n.Importation)
        self.assertEqual(imp.chemin, "u.elg")
        self.assertEqual(imp.alias, "u")

    def test_importation_sans_alias(self):
        imp = analyser_source('importe "u.elg"').instructions[0]
        self.assertIsNone(imp.alias)

    def test_acces_membre_et_appel_methode(self):
        expr = analyser_source("m.f(1)").instructions[0].expression
        self.assertIsInstance(expr, n.AppelMethode)
        self.assertEqual(expr.nom, "f")

        expr2 = analyser_source("soit x = m.valeur").instructions[0].expression
        self.assertIsInstance(expr2, n.AccesMembre)

    def test_precedence_arithmetique(self):
        expr = analyser_source("soit x = 1 + 2 * 3").instructions[0].expression
        self.assertIsInstance(expr, n.OperationBinaire)
        self.assertEqual(expr.operateur, "+")
        self.assertIsInstance(expr.droite, n.OperationBinaire)
        self.assertEqual(expr.droite.operateur, "*")

    def test_erreur_alors_manquant(self):
        with self.assertRaises(ErreurSyntaxe):
            analyser_source("si vrai:\n    affiche 1\n")

    def test_erreur_deux_points_manquant(self):
        with self.assertRaises(ErreurSyntaxe):
            analyser_source("si vrai alors\n    affiche 1\n")

    def test_erreur_expression_invalide(self):
        with self.assertRaises(ErreurSyntaxe):
            analyser_source("soit x = ==")


if __name__ == "__main__":
    unittest.main()
