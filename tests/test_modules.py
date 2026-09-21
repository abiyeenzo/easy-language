import os
import tempfile
import unittest

from easy_language.erreurs import ErreurExecution
from tests.utils import executer


class TestModules(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dossier = self._tmp.name
        self.addCleanup(self._tmp.cleanup)

    def _ecrire(self, nom, contenu):
        chemin = os.path.join(self.dossier, nom)
        os.makedirs(os.path.dirname(chemin), exist_ok=True)
        with open(chemin, "w", encoding="utf-8") as f:
            f.write(contenu)
        return chemin

    def test_import_avec_alias(self):
        self._ecrire("m.elg", "soit PI = 3\nfonction carre(x):\n    retourne x * x\n")
        sortie = executer(
            'importe "m.elg" comme math\n'
            'affiche math.PI\n'
            'affiche math.carre(4)\n',
            self.dossier,
        )
        self.assertEqual(sortie, "3\n16\n")

    def test_import_sans_alias_utilise_nom_fichier(self):
        self._ecrire("outils.elg", "fonction f():\n    retourne 1\n")
        sortie = executer('importe "outils.elg"\naffiche outils.f()\n', self.dossier)
        self.assertEqual(sortie, "1\n")

    def test_import_chemin_relatif_sous_dossier(self):
        self._ecrire("lib/m.elg", "fonction f():\n    retourne 5\n")
        sortie = executer('importe "lib/m.elg" comme m\naffiche m.f()\n', self.dossier)
        self.assertEqual(sortie, "5\n")

    def test_import_imbrique(self):
        self._ecrire("base.elg", "fonction un():\n    retourne 1\n")
        self._ecrire("haut.elg", 'importe "base.elg" comme base\nfonction deux():\n    retourne base.un() + 1\n')
        sortie = executer('importe "haut.elg" comme h\naffiche h.deux()\n', self.dossier)
        self.assertEqual(sortie, "2\n")

    def test_import_circulaire_detecte(self):
        self._ecrire("a.elg", 'importe "b.elg" comme b\nfonction f():\n    retourne 1\n')
        self._ecrire("b.elg", 'importe "a.elg" comme a\nfonction g():\n    retourne 2\n')
        with self.assertRaises(ErreurExecution):
            executer('importe "a.elg" comme a\n', self.dossier)

    def test_import_fichier_inexistant(self):
        with self.assertRaises(ErreurExecution):
            executer('importe "nexistepas.elg"\n', self.dossier)

    def test_acces_membre_inexistant(self):
        self._ecrire("m.elg", "soit x = 1\n")
        with self.assertRaises(ErreurExecution):
            executer('importe "m.elg" comme m\naffiche m.inconnu\n', self.dossier)

    def test_appel_membre_non_fonction(self):
        self._ecrire("m.elg", "soit x = 1\n")
        with self.assertRaises(ErreurExecution):
            executer('importe "m.elg" comme m\naffiche m.x(1)\n', self.dossier)

    def test_acces_membre_sur_non_module(self):
        with self.assertRaises(ErreurExecution):
            executer('soit x = 5\naffiche x.nom\n', self.dossier)

    def test_erreur_dans_module_remonte(self):
        self._ecrire("m.elg", "affiche 1 / 0\n")
        with self.assertRaises(ErreurExecution):
            executer('importe "m.elg" comme m\n', self.dossier)


if __name__ == "__main__":
    unittest.main()
