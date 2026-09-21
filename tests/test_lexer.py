import unittest

from easy_language.erreurs import ErreurLexicale
from easy_language.lexer import Lexer


def types(source):
    return [j.type for j in Lexer(source).tokeniser()]


class TestLexer(unittest.TestCase):
    def test_declaration_simple(self):
        self.assertEqual(
            types("soit x = 5"),
            ["SOIT", "IDENT", "EGAL", "NOMBRE", "NOUVELLE_LIGNE", "FIN"],
        )

    def test_nombre_flottant(self):
        jetons = Lexer("soit x = 3.14").tokeniser()
        self.assertEqual(jetons[3].valeur, 3.14)
        self.assertIsInstance(jetons[3].valeur, float)

    def test_nombre_entier(self):
        jetons = Lexer("soit x = 42").tokeniser()
        self.assertEqual(jetons[3].valeur, 42)
        self.assertIsInstance(jetons[3].valeur, int)

    def test_chaine_simple(self):
        jetons = Lexer('affiche "bonjour"').tokeniser()
        self.assertEqual(jetons[1].type, "TEXTE")
        self.assertEqual(jetons[1].valeur, "bonjour")

    def test_chaine_avec_echappement(self):
        jetons = Lexer('affiche "a\\nb\\t\\"c\\""').tokeniser()
        self.assertEqual(jetons[1].valeur, 'a\nb\t"c"')

    def test_chaine_non_terminee(self):
        with self.assertRaises(ErreurLexicale):
            Lexer('affiche "sans fin').tokeniser()

    def test_commentaire_ignore(self):
        self.assertEqual(
            types("soit x = 1 # un commentaire"),
            ["SOIT", "IDENT", "EGAL", "NOMBRE", "NOUVELLE_LIGNE", "FIN"],
        )

    def test_ligne_vide_ignoree(self):
        self.assertEqual(types("\n\nsoit x = 1\n\n"), ["SOIT", "IDENT", "EGAL", "NOMBRE", "NOUVELLE_LIGNE", "FIN"])

    def test_indentation_indent_dedent(self):
        source = "si vrai alors:\n    affiche 1\naffiche 2\n"
        t = types(source)
        self.assertEqual(t.count("INDENT"), 1)
        self.assertEqual(t.count("DEDENT"), 1)
        self.assertLess(t.index("INDENT"), t.index("DEDENT"))

    def test_indentation_incoherente(self):
        source = "si vrai alors:\n    affiche 1\n  affiche 2\n"
        with self.assertRaises(ErreurLexicale):
            Lexer(source).tokeniser()

    def test_tabulation_interdite(self):
        with self.assertRaises(ErreurLexicale):
            Lexer("si vrai alors:\n\taffiche 1\n").tokeniser()

    def test_caractere_inattendu(self):
        with self.assertRaises(ErreurLexicale):
            Lexer("soit x = 5 @ 3").tokeniser()

    def test_mots_cles_reconnus(self):
        self.assertEqual(
            types("si sinon tantque pour fonction retourne vrai faux rien et ou non arrete continue"),
            [
                "SI", "SINON", "TANTQUE", "POUR", "FONCTION", "RETOURNE",
                "VRAI", "FAUX", "RIEN", "ET", "OU", "NON", "ARRETE", "CONTINUE",
                "NOUVELLE_LIGNE", "FIN",
            ],
        )

    def test_symboles_operateurs(self):
        self.assertEqual(
            types("== != <= >= + - * / % = < > ( ) [ ] : ."),
            [
                "EGAL_EGAL", "DIFFERENT", "INF_EGAL", "SUP_EGAL", "PLUS", "MOINS",
                "FOIS", "DIVISE", "MODULO", "EGAL", "INF", "SUP", "PARO", "PARF",
                "CROCHET_O", "CROCHET_F", "DEUX_POINTS", "POINT",
                "NOUVELLE_LIGNE", "FIN",
            ],
        )

    def test_numeros_de_ligne(self):
        jetons = Lexer("soit x = 1\nsoit y = 2\n").tokeniser()
        lignes = [j.ligne for j in jetons if j.type == "SOIT"]
        self.assertEqual(lignes, [1, 2])


if __name__ == "__main__":
    unittest.main()
