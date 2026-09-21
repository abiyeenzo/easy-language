from .erreurs import ErreurSyntaxe
from . import noeuds as n

DEBUT_EXPRESSION = {
    "NOMBRE", "TEXTE", "IDENT", "PARO", "CROCHET_O", "VRAI", "FAUX", "RIEN", "NON", "MOINS",
}


class Analyseur:
    def __init__(self, jetons):
        self.jetons = jetons
        self.position = 0

    def _actuel(self):
        return self.jetons[self.position]

    def _type_actuel(self):
        return self._actuel().type

    def _avancer(self):
        jeton = self.jetons[self.position]
        self.position += 1
        return jeton

    def _attendre(self, type_, message=None):
        if self._type_actuel() != type_:
            raise ErreurSyntaxe(
                message or f"attendu {type_}, trouve {self._type_actuel()} ({self._actuel().valeur!r})",
                self._actuel().ligne,
            )
        return self._avancer()

    def _correspond(self, *types):
        return self._type_actuel() in types

    def analyser(self):
        instructions = []
        self._ignorer_lignes_vides()
        while not self._correspond("FIN"):
            instructions.append(self._instruction())
            self._ignorer_lignes_vides()
        return n.Programme(instructions)

    def _ignorer_lignes_vides(self):
        while self._correspond("NOUVELLE_LIGNE"):
            self._avancer()

    def _fin_instruction(self):
        if self._correspond("FIN"):
            return
        self._attendre("NOUVELLE_LIGNE", "fin de ligne attendue")

    def _bloc(self):
        self._attendre("NOUVELLE_LIGNE", "un bloc doit commencer sur une nouvelle ligne")
        self._ignorer_lignes_vides()
        self._attendre("INDENT", "bloc indente attendu")
        instructions = []
        self._ignorer_lignes_vides()
        while not self._correspond("DEDENT", "FIN"):
            instructions.append(self._instruction())
            self._ignorer_lignes_vides()
        if self._correspond("DEDENT"):
            self._avancer()
        return instructions

    def _instruction(self):
        type_ = self._type_actuel()
        if type_ == "SOIT":
            return self._declaration()
        if type_ == "AFFICHE":
            return self._affiche()
        if type_ == "DEMANDE":
            return self._demande()
        if type_ == "SI":
            return self._si()
        if type_ == "TANTQUE":
            return self._tantque()
        if type_ == "POUR":
            return self._pour()
        if type_ == "FONCTION":
            return self._definition_fonction()
        if type_ == "RETOURNE":
            return self._retourne()
        if type_ == "ARRETE":
            ligne = self._avancer().ligne
            self._fin_instruction()
            return n.Arrete(ligne)
        if type_ == "CONTINUE":
            ligne = self._avancer().ligne
            self._fin_instruction()
            return n.Continue(ligne)
        if type_ == "IMPORTE":
            return self._importation()
        return self._expression_ou_affectation()

    def _importation(self):
        ligne = self._avancer().ligne
        chemin = self._attendre("TEXTE", "chemin de fichier (texte) attendu apres 'importe'").valeur
        alias = None
        if self._correspond("COMME"):
            self._avancer()
            alias = self._attendre("IDENT", "nom d'alias attendu apres 'comme'").valeur
        self._fin_instruction()
        return n.Importation(chemin, alias, ligne)

    def _declaration(self):
        ligne = self._avancer().ligne
        nom = self._attendre("IDENT", "nom de variable attendu apres 'soit'").valeur
        self._attendre("EGAL", "'=' attendu")
        expression = self._expression()
        self._fin_instruction()
        return n.Declaration(nom, expression, ligne)

    def _expression_ou_affectation(self):
        ligne = self._actuel().ligne
        cible = self._expression()

        if self._correspond("EGAL"):
            self._avancer()
            valeur = self._expression()
            self._fin_instruction()
            if isinstance(cible, n.Variable):
                return n.Affectation(cible.nom, valeur, ligne)
            if isinstance(cible, n.Indexation):
                return n.AffectationIndex(cible.cible, cible.index, valeur, ligne)
            raise ErreurSyntaxe("cible d'affectation invalide", ligne)

        self._fin_instruction()
        return n.ExpressionInstruction(cible, ligne)

    def _affiche(self):
        ligne = self._avancer().ligne
        expressions = self._liste_expressions()
        self._fin_instruction()
        return n.Affiche(expressions, ligne)

    def _demande(self):
        ligne = self._avancer().ligne
        nom = self._attendre("IDENT", "nom de variable attendu apres 'demande'").valeur
        invite = None
        if self._correspond("TEXTE"):
            invite = self._avancer().valeur
        self._fin_instruction()
        return n.Demande(nom, invite, ligne)

    def _condition_parenthesee_optionnelle(self):
        return self._expression()

    def _si(self):
        ligne = self._avancer().ligne
        branches = []
        condition = self._condition_parenthesee_optionnelle()
        self._attendre("ALORS", "'alors' attendu apres la condition")
        self._attendre("DEUX_POINTS", "':' attendu apres 'alors'")
        branches.append((condition, self._bloc()))

        sinon = None
        while self._correspond("SINON"):
            self._avancer()
            if self._correspond("SI"):
                self._avancer()
                cond = self._condition_parenthesee_optionnelle()
                self._attendre("ALORS", "'alors' attendu apres la condition")
                self._attendre("DEUX_POINTS")
                branches.append((cond, self._bloc()))
            else:
                self._attendre("DEUX_POINTS", "':' attendu apres 'sinon'")
                sinon = self._bloc()
                break
        return n.Si(branches, sinon, ligne)

    def _tantque(self):
        ligne = self._avancer().ligne
        condition = self._expression()
        self._attendre("ALORS", "'alors' attendu apres la condition")
        self._attendre("DEUX_POINTS")
        return n.TantQue(condition, self._bloc(), ligne)

    def _pour(self):
        ligne = self._avancer().ligne

        if self._correspond("CHAQUE"):
            self._avancer()
            variable = self._attendre("IDENT", "nom de variable attendu apres 'chaque'").valeur
            self._attendre("DANS", "'dans' attendu")
            iterable = self._expression()
            self._attendre("ALORS", "'alors' attendu")
            self._attendre("DEUX_POINTS")
            return n.PourChaque(variable, iterable, self._bloc(), ligne)

        variable = self._attendre("IDENT", "nom de variable attendu apres 'pour'").valeur
        self._attendre("DE", "'de' attendu")
        depart = self._expression()
        self._attendre("JUSQUA", "'jusqua' attendu")
        fin = self._expression()
        pas = None
        if self._correspond("PAS"):
            self._avancer()
            pas = self._expression()
        self._attendre("ALORS", "'alors' attendu")
        self._attendre("DEUX_POINTS")
        return n.Pour(variable, depart, fin, pas, self._bloc(), ligne)

    def _definition_fonction(self):
        ligne = self._avancer().ligne
        nom = self._attendre("IDENT", "nom de fonction attendu").valeur
        self._attendre("PARO", "'(' attendu apres le nom de la fonction")
        parametres = []
        while self._correspond("IDENT"):
            parametres.append(self._avancer().valeur)
        self._attendre("PARF", "')' attendu")
        self._attendre("DEUX_POINTS", "':' attendu")
        return n.DefinitionFonction(nom, parametres, self._bloc(), ligne)

    def _retourne(self):
        ligne = self._avancer().ligne
        expression = None
        if self._correspond("NOUVELLE_LIGNE", "FIN"):
            expression = None
        else:
            expression = self._expression()
        self._fin_instruction()
        return n.Retourne(expression, ligne)

    def _liste_expressions(self):
        expressions = [self._expression()]
        while self._type_actuel() in DEBUT_EXPRESSION:
            expressions.append(self._expression())
        return expressions

    # --- expressions, par precedence croissante ---

    def _expression(self):
        return self._ou()

    def _ou(self):
        gauche = self._et()
        while self._correspond("OU"):
            ligne = self._avancer().ligne
            droite = self._et()
            gauche = n.OperationBinaire(gauche, "ou", droite, ligne)
        return gauche

    def _et(self):
        gauche = self._non()
        while self._correspond("ET"):
            ligne = self._avancer().ligne
            droite = self._non()
            gauche = n.OperationBinaire(gauche, "et", droite, ligne)
        return gauche

    def _non(self):
        if self._correspond("NON"):
            ligne = self._avancer().ligne
            return n.OperationUnaire("non", self._non(), ligne)
        return self._comparaison()

    def _comparaison(self):
        gauche = self._addition()
        while self._correspond("EGAL_EGAL", "DIFFERENT", "INF", "SUP", "INF_EGAL", "SUP_EGAL"):
            jeton = self._avancer()
            droite = self._addition()
            gauche = n.OperationBinaire(gauche, jeton.valeur, droite, jeton.ligne)
        return gauche

    def _addition(self):
        gauche = self._terme()
        while self._correspond("PLUS", "MOINS"):
            jeton = self._avancer()
            droite = self._terme()
            gauche = n.OperationBinaire(gauche, jeton.valeur, droite, jeton.ligne)
        return gauche

    def _terme(self):
        gauche = self._facteur()
        while self._correspond("FOIS", "DIVISE", "MODULO"):
            jeton = self._avancer()
            droite = self._facteur()
            gauche = n.OperationBinaire(gauche, jeton.valeur, droite, jeton.ligne)
        return gauche

    def _facteur(self):
        if self._correspond("MOINS"):
            ligne = self._avancer().ligne
            return n.OperationUnaire("-", self._facteur(), ligne)
        return self._primaire()

    def _primaire(self):
        cible = self._primaire_base()
        if isinstance(cible, n.Liste):
            # un tableau litteral ne s'indexe pas directement (ambiguite avec
            # une liste d'elements adjacente, ex: [[1 2] [3 4]]); passez par
            # une variable: soit m = [1 2] puis m[0]
            return cible
        while self._correspond("CROCHET_O", "POINT"):
            if self._correspond("CROCHET_O"):
                ligne = self._avancer().ligne
                index = self._expression()
                self._attendre("CROCHET_F", "']' attendu")
                cible = n.Indexation(cible, index, ligne)
            else:
                ligne = self._avancer().ligne
                nom = self._attendre("IDENT", "nom de membre attendu apres '.'").valeur
                if self._correspond("PARO"):
                    self._avancer()
                    arguments = []
                    while self._type_actuel() in DEBUT_EXPRESSION:
                        arguments.append(self._expression())
                    self._attendre("PARF", "')' attendu apres les arguments")
                    cible = n.AppelMethode(cible, nom, arguments, ligne)
                else:
                    cible = n.AccesMembre(cible, nom, ligne)
        return cible

    def _primaire_base(self):
        jeton = self._actuel()

        if jeton.type == "CROCHET_O":
            ligne = self._avancer().ligne
            elements = []
            while self._type_actuel() in DEBUT_EXPRESSION:
                elements.append(self._expression())
            self._attendre("CROCHET_F", "']' attendu pour fermer la liste")
            return n.Liste(elements, ligne)

        if jeton.type == "NOMBRE":
            self._avancer()
            return n.Litteral(jeton.valeur, jeton.ligne)

        if jeton.type == "TEXTE":
            self._avancer()
            return n.Litteral(jeton.valeur, jeton.ligne)

        if jeton.type == "VRAI":
            self._avancer()
            return n.Litteral(True, jeton.ligne)

        if jeton.type == "FAUX":
            self._avancer()
            return n.Litteral(False, jeton.ligne)

        if jeton.type == "RIEN":
            self._avancer()
            return n.Litteral(None, jeton.ligne)

        if jeton.type == "PARO":
            self._avancer()
            expression = self._expression()
            self._attendre("PARF", "')' attendu")
            return expression

        if jeton.type == "IDENT":
            self._avancer()
            if self._correspond("PARO"):
                self._avancer()
                arguments = []
                while self._type_actuel() in DEBUT_EXPRESSION:
                    arguments.append(self._expression())
                self._attendre("PARF", "')' attendu apres les arguments")
                return n.AppelFonction(jeton.valeur, arguments, jeton.ligne)
            return n.Variable(jeton.valeur, jeton.ligne)

        raise ErreurSyntaxe(f"expression inattendue: {jeton.type} ({jeton.valeur!r})", jeton.ligne)
