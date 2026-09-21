from .interpreteur import _formater_pour_affichage


class ArretDebogueur(Exception):
    """Leve quand l'utilisateur tape 'q' pour quitter le debogueur."""


class Debogueur:
    """Se branche sur Interpreteur.avant_instruction pour un pas-a-pas ligne par ligne."""

    def __init__(self, interpreteur, lignes_source, points_arret=None):
        self.interpreteur = interpreteur
        self.lignes_source = lignes_source
        self.points_arret = set(points_arret or [])
        self.mode = "pas" if not self.points_arret else "continuer"
        interpreteur.avant_instruction = self._avant_instruction

    def _avant_instruction(self, noeud, env):
        ligne = getattr(noeud, "ligne", None)
        doit_arreter = self.mode == "pas" or (ligne in self.points_arret)
        if not doit_arreter:
            return
        self._afficher_contexte(ligne)
        self._boucle_commandes(env)

    def _afficher_contexte(self, ligne):
        texte = ""
        if ligne and 0 < ligne <= len(self.lignes_source):
            texte = self.lignes_source[ligne - 1].strip()
        print(f"-> ligne {ligne}: {texte}")

    def _boucle_commandes(self, env):
        while True:
            try:
                commande = input("(debogueur) ").strip()
            except EOFError:
                self.mode = "continuer"
                return

            if commande in ("", "n", "suivant"):
                self.mode = "pas"
                return
            if commande in ("c", "continuer"):
                self.mode = "continuer"
                return
            if commande in ("q", "quitter"):
                raise ArretDebogueur()
            if commande in ("v", "variables"):
                self._afficher_variables(env)
                continue
            if commande in ("b", "arrets"):
                print("points d'arret:", sorted(self.points_arret) or "aucun")
                continue
            if commande.startswith("ba "):
                self._ajouter_arret(commande[3:].strip())
                continue
            if commande.startswith("br "):
                self._retirer_arret(commande[3:].strip())
                continue
            if commande.startswith("p ") or commande.startswith("affiche "):
                nom = commande.split(" ", 1)[1].strip()
                self._afficher_variable(nom, env)
                continue
            if commande in ("aide", "?", "h"):
                self._afficher_aide()
                continue

            print(f"commande inconnue: '{commande}' (tapez 'aide')")

    def _ajouter_arret(self, valeur):
        try:
            self.points_arret.add(int(valeur))
            print(f"point d'arret ajoute a la ligne {valeur}")
        except ValueError:
            print(f"numero de ligne invalide: {valeur!r}")

    def _retirer_arret(self, valeur):
        try:
            self.points_arret.discard(int(valeur))
            print(f"point d'arret retire de la ligne {valeur}")
        except ValueError:
            print(f"numero de ligne invalide: {valeur!r}")

    def _afficher_variable(self, nom, env):
        e = env
        while e is not None:
            if nom in e.valeurs:
                print(f"{nom} = {_formater_pour_affichage(e.valeurs[nom])}")
                return
            e = e.parent
        print(f"variable inconnue: '{nom}'")

    def _afficher_variables(self, env):
        vus = set()
        e = env
        while e is not None:
            for nom, valeur in e.valeurs.items():
                if nom not in vus:
                    vus.add(nom)
                    print(f"{nom} = {_formater_pour_affichage(valeur)}")
            e = e.parent

    def _afficher_aide(self):
        print(
            "commandes:\n"
            "  n, suivant       executer la ligne courante et s'arreter a la prochaine\n"
            "  c, continuer      continuer jusqu'au prochain point d'arret\n"
            "  ba <ligne>        ajouter un point d'arret\n"
            "  br <ligne>        retirer un point d'arret\n"
            "  b, arrets         lister les points d'arret\n"
            "  v, variables      afficher toutes les variables visibles\n"
            "  p <nom>           afficher la valeur d'une variable\n"
            "  q, quitter        arreter l'execution\n"
        )
