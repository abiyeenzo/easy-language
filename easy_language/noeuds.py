class Noeud:
    pass


class Programme(Noeud):
    def __init__(self, instructions):
        self.instructions = instructions


class Declaration(Noeud):
    def __init__(self, nom, expression, ligne):
        self.nom = nom
        self.expression = expression
        self.ligne = ligne


class Affectation(Noeud):
    def __init__(self, nom, expression, ligne):
        self.nom = nom
        self.expression = expression
        self.ligne = ligne


class Affiche(Noeud):
    def __init__(self, expressions, ligne):
        self.expressions = expressions
        self.ligne = ligne


class Demande(Noeud):
    def __init__(self, nom, invite, ligne):
        self.nom = nom
        self.invite = invite
        self.ligne = ligne


class Si(Noeud):
    def __init__(self, branches, sinon, ligne):
        # branches: liste de (condition, bloc)
        self.branches = branches
        self.sinon = sinon
        self.ligne = ligne


class TantQue(Noeud):
    def __init__(self, condition, bloc, ligne):
        self.condition = condition
        self.bloc = bloc
        self.ligne = ligne


class Pour(Noeud):
    def __init__(self, variable, depart, fin, pas, bloc, ligne):
        self.variable = variable
        self.depart = depart
        self.fin = fin
        self.pas = pas
        self.bloc = bloc
        self.ligne = ligne


class DefinitionFonction(Noeud):
    def __init__(self, nom, parametres, bloc, ligne):
        self.nom = nom
        self.parametres = parametres
        self.bloc = bloc
        self.ligne = ligne


class Retourne(Noeud):
    def __init__(self, expression, ligne):
        self.expression = expression
        self.ligne = ligne


class Arrete(Noeud):
    def __init__(self, ligne):
        self.ligne = ligne


class Continue(Noeud):
    def __init__(self, ligne):
        self.ligne = ligne


class ExpressionInstruction(Noeud):
    def __init__(self, expression, ligne):
        self.expression = expression
        self.ligne = ligne


class Litteral(Noeud):
    def __init__(self, valeur, ligne):
        self.valeur = valeur
        self.ligne = ligne


class Variable(Noeud):
    def __init__(self, nom, ligne):
        self.nom = nom
        self.ligne = ligne


class OperationBinaire(Noeud):
    def __init__(self, gauche, operateur, droite, ligne):
        self.gauche = gauche
        self.operateur = operateur
        self.droite = droite
        self.ligne = ligne


class OperationUnaire(Noeud):
    def __init__(self, operateur, operande, ligne):
        self.operateur = operateur
        self.operande = operande
        self.ligne = ligne


class AppelFonction(Noeud):
    def __init__(self, nom, arguments, ligne):
        self.nom = nom
        self.arguments = arguments
        self.ligne = ligne


class Liste(Noeud):
    def __init__(self, elements, ligne):
        self.elements = elements
        self.ligne = ligne


class Indexation(Noeud):
    def __init__(self, cible, index, ligne):
        self.cible = cible
        self.index = index
        self.ligne = ligne


class AffectationIndex(Noeud):
    def __init__(self, cible, index, expression, ligne):
        self.cible = cible
        self.index = index
        self.expression = expression
        self.ligne = ligne


class PourChaque(Noeud):
    def __init__(self, variable, iterable, bloc, ligne):
        self.variable = variable
        self.iterable = iterable
        self.bloc = bloc
        self.ligne = ligne


class Importation(Noeud):
    def __init__(self, chemin, alias, ligne):
        self.chemin = chemin
        self.alias = alias
        self.ligne = ligne


class AccesMembre(Noeud):
    def __init__(self, cible, nom, ligne):
        self.cible = cible
        self.nom = nom
        self.ligne = ligne


class AppelMethode(Noeud):
    def __init__(self, cible, nom, arguments, ligne):
        self.cible = cible
        self.nom = nom
        self.arguments = arguments
        self.ligne = ligne
