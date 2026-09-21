import os

from .erreurs import ErreurEasyLang, ErreurExecution
from . import noeuds as n


class SignalRetour(Exception):
    def __init__(self, valeur):
        self.valeur = valeur


class SignalArrete(Exception):
    pass


class SignalContinue(Exception):
    pass


class Environnement:
    def __init__(self, parent=None):
        self.valeurs = {}
        self.parent = parent

    def definir(self, nom, valeur):
        self.valeurs[nom] = valeur

    def obtenir(self, nom, ligne):
        env = self
        while env is not None:
            if nom in env.valeurs:
                return env.valeurs[nom]
            env = env.parent
        raise ErreurExecution(f"variable inconnue: '{nom}'", ligne)

    def assigner(self, nom, valeur, ligne):
        env = self
        while env is not None:
            if nom in env.valeurs:
                env.valeurs[nom] = valeur
                return
            env = env.parent
        raise ErreurExecution(f"impossible d'assigner, variable non declaree: '{nom}' (utilisez 'soit')", ligne)


class FonctionEasyLang:
    def __init__(self, nom, parametres, bloc, environnement_definition):
        self.nom = nom
        self.parametres = parametres
        self.bloc = bloc
        self.environnement_definition = environnement_definition


class Module:
    def __init__(self, nom, environnement):
        self.nom = nom
        self.environnement = environnement


def _formater_pour_affichage(valeur):
    if valeur is None:
        return "rien"
    if valeur is True:
        return "vrai"
    if valeur is False:
        return "faux"
    if isinstance(valeur, list):
        return "[" + " ".join(_formater_pour_affichage(e) for e in valeur) + "]"
    if isinstance(valeur, Module):
        return f"<module {valeur.nom}>"
    if isinstance(valeur, FonctionEasyLang):
        return f"<fonction {valeur.nom}>"
    return str(valeur)


def _est_vrai(valeur):
    if valeur is None or valeur is False:
        return False
    if isinstance(valeur, list):
        return len(valeur) > 0
    if valeur == 0:
        return False
    if valeur == "":
        return False
    return True


class Interpreteur:
    def __init__(self):
        self.globales = Environnement()
        self._installer_fonctions_natives()
        self.dossier_actuel = os.getcwd()
        self.modules_charges = {}
        self.modules_en_cours = set()
        self.avant_instruction = None

    def _installer_fonctions_natives(self):
        def longueur(valeur, ligne):
            try:
                return len(valeur)
            except TypeError:
                raise ErreurExecution(f"longueur() ne s'applique pas a {valeur!r}", ligne)

        def entier(valeur, ligne):
            try:
                return int(valeur)
            except (TypeError, ValueError):
                raise ErreurExecution(f"impossible de convertir {valeur!r} en entier", ligne)

        def nombre(valeur, ligne):
            try:
                if isinstance(valeur, str) and "." in valeur:
                    return float(valeur)
                return int(valeur)
            except (TypeError, ValueError):
                raise ErreurExecution(f"impossible de convertir {valeur!r} en nombre", ligne)

        def texte(valeur, ligne):
            return _formater_pour_affichage(valeur)

        def ajoute(liste, valeur, ligne):
            if not isinstance(liste, list):
                raise ErreurExecution(f"ajoute() attend une liste, recu {liste!r}", ligne)
            liste.append(valeur)
            return None

        def retire(liste, index, ligne):
            if not isinstance(liste, list):
                raise ErreurExecution(f"retire() attend une liste, recu {liste!r}", ligne)
            if not isinstance(index, int) or isinstance(index, bool):
                raise ErreurExecution(f"retire() attend un index entier, recu {index!r}", ligne)
            try:
                return liste.pop(index)
            except IndexError:
                raise ErreurExecution(f"index hors limites: {index}", ligne)

        def contient(liste, valeur, ligne):
            if not isinstance(liste, (list, str)):
                raise ErreurExecution(f"contient() attend une liste ou un texte, recu {liste!r}", ligne)
            return valeur in liste

        self.natives = {
            "longueur": (1, longueur),
            "entier": (1, entier),
            "nombre": (1, nombre),
            "texte": (1, texte),
            "ajoute": (2, ajoute),
            "retire": (2, retire),
            "contient": (2, contient),
        }

    def executer(self, programme, dossier=None):
        if dossier is not None:
            self.dossier_actuel = dossier
        self._executer_bloc(programme.instructions, self.globales)

    def _executer_bloc(self, instructions, environnement):
        for instruction in instructions:
            self._executer_instruction(instruction, environnement)

    def _executer_instruction(self, instruction, env):
        if self.avant_instruction is not None:
            self.avant_instruction(instruction, env)
        methode = getattr(self, f"_exec_{type(instruction).__name__}")
        methode(instruction, env)

    def _exec_Declaration(self, noeud, env):
        env.definir(noeud.nom, self._evaluer(noeud.expression, env))

    def _exec_Affectation(self, noeud, env):
        env.assigner(noeud.nom, self._evaluer(noeud.expression, env), noeud.ligne)

    def _exec_AffectationIndex(self, noeud, env):
        conteneur = self._evaluer(noeud.cible, env)
        index = self._evaluer(noeud.index, env)
        valeur = self._evaluer(noeud.expression, env)
        if not isinstance(conteneur, list):
            raise ErreurExecution(f"impossible d'indexer {conteneur!r}", noeud.ligne)
        if not isinstance(index, int) or isinstance(index, bool):
            raise ErreurExecution(f"index invalide: {index!r}", noeud.ligne)
        try:
            conteneur[index] = valeur
        except IndexError:
            raise ErreurExecution(f"index hors limites: {index}", noeud.ligne)

    def _exec_Importation(self, noeud, env):
        chemin_absolu = os.path.normpath(os.path.join(self.dossier_actuel, noeud.chemin))
        alias = noeud.alias or os.path.splitext(os.path.basename(noeud.chemin))[0]

        if chemin_absolu in self.modules_charges:
            env.definir(alias, self.modules_charges[chemin_absolu])
            return

        if chemin_absolu in self.modules_en_cours:
            raise ErreurExecution(f"import circulaire detecte: '{noeud.chemin}'", noeud.ligne)

        try:
            with open(chemin_absolu, "r", encoding="utf-8") as f:
                source = f.read()
        except OSError as e:
            raise ErreurExecution(f"impossible d'importer '{noeud.chemin}': {e}", noeud.ligne)

        from .lexer import Lexer
        from .analyseur import Analyseur

        try:
            jetons = Lexer(source).tokeniser()
            programme_module = Analyseur(jetons).analyser()
        except ErreurEasyLang as e:
            raise ErreurExecution(
                f"erreur dans le module importe '{noeud.chemin}' (ligne {e.ligne}): {e.message}", noeud.ligne
            )

        env_module = Environnement()
        dossier_precedent = self.dossier_actuel
        self.modules_en_cours.add(chemin_absolu)
        self.dossier_actuel = os.path.dirname(chemin_absolu)
        try:
            self._executer_bloc(programme_module.instructions, env_module)
        except ErreurEasyLang as e:
            raise ErreurExecution(
                f"erreur dans le module importe '{noeud.chemin}' (ligne {e.ligne}): {e.message}", noeud.ligne
            )
        finally:
            self.dossier_actuel = dossier_precedent
            self.modules_en_cours.discard(chemin_absolu)

        module = Module(alias, env_module)
        self.modules_charges[chemin_absolu] = module
        env.definir(alias, module)

    def _exec_Affiche(self, noeud, env):
        valeurs = [self._evaluer(expr, env) for expr in noeud.expressions]
        print(" ".join(_formater_pour_affichage(v) for v in valeurs))

    def _exec_Demande(self, noeud, env):
        invite = noeud.invite if noeud.invite is not None else ""
        brut = input(invite)
        try:
            valeur = int(brut)
        except ValueError:
            try:
                valeur = float(brut)
            except ValueError:
                valeur = brut
        env.definir(noeud.nom, valeur)

    def _exec_Si(self, noeud, env):
        for condition, bloc in noeud.branches:
            if _est_vrai(self._evaluer(condition, env)):
                self._executer_bloc(bloc, Environnement(env))
                return
        if noeud.sinon is not None:
            self._executer_bloc(noeud.sinon, Environnement(env))

    def _exec_TantQue(self, noeud, env):
        while _est_vrai(self._evaluer(noeud.condition, env)):
            try:
                self._executer_bloc(noeud.bloc, Environnement(env))
            except SignalArrete:
                break
            except SignalContinue:
                continue

    def _exec_Pour(self, noeud, env):
        depart = self._evaluer(noeud.depart, env)
        fin = self._evaluer(noeud.fin, env)
        pas = self._evaluer(noeud.pas, env) if noeud.pas is not None else 1
        if pas == 0:
            raise ErreurExecution("le pas d'une boucle 'pour' ne peut pas etre zero", noeud.ligne)

        valeur = depart
        while (pas > 0 and valeur <= fin) or (pas < 0 and valeur >= fin):
            sous_env = Environnement(env)
            sous_env.definir(noeud.variable, valeur)
            try:
                self._executer_bloc(noeud.bloc, sous_env)
            except SignalArrete:
                break
            except SignalContinue:
                pass
            valeur += pas

    def _exec_PourChaque(self, noeud, env):
        iterable = self._evaluer(noeud.iterable, env)
        if not isinstance(iterable, (list, str)):
            raise ErreurExecution(f"'pour chaque' attend une liste ou un texte, recu {iterable!r}", noeud.ligne)
        for element in iterable:
            sous_env = Environnement(env)
            sous_env.definir(noeud.variable, element)
            try:
                self._executer_bloc(noeud.bloc, sous_env)
            except SignalArrete:
                break
            except SignalContinue:
                continue

    def _exec_DefinitionFonction(self, noeud, env):
        env.definir(noeud.nom, FonctionEasyLang(noeud.nom, noeud.parametres, noeud.bloc, env))

    def _exec_Retourne(self, noeud, env):
        valeur = self._evaluer(noeud.expression, env) if noeud.expression is not None else None
        raise SignalRetour(valeur)

    def _exec_Arrete(self, noeud, env):
        raise SignalArrete()

    def _exec_Continue(self, noeud, env):
        raise SignalContinue()

    def _exec_ExpressionInstruction(self, noeud, env):
        self._evaluer(noeud.expression, env)

    # --- evaluation d'expressions ---

    def _evaluer(self, noeud, env):
        methode = getattr(self, f"_eval_{type(noeud).__name__}")
        return methode(noeud, env)

    def _eval_Liste(self, noeud, env):
        return [self._evaluer(element, env) for element in noeud.elements]

    def _eval_Indexation(self, noeud, env):
        conteneur = self._evaluer(noeud.cible, env)
        index = self._evaluer(noeud.index, env)
        if not isinstance(conteneur, (list, str)):
            raise ErreurExecution(f"impossible d'indexer {conteneur!r}", noeud.ligne)
        if not isinstance(index, int) or isinstance(index, bool):
            raise ErreurExecution(f"index invalide: {index!r}", noeud.ligne)
        try:
            return conteneur[index]
        except IndexError:
            raise ErreurExecution(f"index hors limites: {index}", noeud.ligne)

    def _eval_Litteral(self, noeud, env):
        return noeud.valeur

    def _eval_Variable(self, noeud, env):
        return env.obtenir(noeud.nom, noeud.ligne)

    def _eval_OperationUnaire(self, noeud, env):
        valeur = self._evaluer(noeud.operande, env)
        if noeud.operateur == "-":
            if not isinstance(valeur, (int, float)) or isinstance(valeur, bool):
                raise ErreurExecution(f"impossible d'appliquer '-' a {valeur!r}", noeud.ligne)
            return -valeur
        if noeud.operateur == "non":
            return not _est_vrai(valeur)
        raise ErreurExecution(f"operateur unaire inconnu: {noeud.operateur}", noeud.ligne)

    def _eval_OperationBinaire(self, noeud, env):
        op = noeud.operateur

        if op == "et":
            gauche = self._evaluer(noeud.gauche, env)
            if not _est_vrai(gauche):
                return gauche
            return self._evaluer(noeud.droite, env)

        if op == "ou":
            gauche = self._evaluer(noeud.gauche, env)
            if _est_vrai(gauche):
                return gauche
            return self._evaluer(noeud.droite, env)

        gauche = self._evaluer(noeud.gauche, env)
        droite = self._evaluer(noeud.droite, env)
        ligne = noeud.ligne

        if op == "+":
            if isinstance(gauche, list) and isinstance(droite, list):
                return gauche + droite
            if isinstance(gauche, str) or isinstance(droite, str):
                return _formater_pour_affichage(gauche) + _formater_pour_affichage(droite)
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche + droite
        if op == "-":
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche - droite
        if op == "*":
            if isinstance(gauche, str) and isinstance(droite, int):
                return gauche * droite
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche * droite
        if op == "/":
            self._verifier_nombres(gauche, droite, op, ligne)
            if droite == 0:
                raise ErreurExecution("division par zero", ligne)
            resultat = gauche / droite
            return resultat
        if op == "%":
            self._verifier_nombres(gauche, droite, op, ligne)
            if droite == 0:
                raise ErreurExecution("division par zero", ligne)
            return gauche % droite
        if op == "==":
            return gauche == droite
        if op == "!=":
            return gauche != droite
        if op == "<":
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche < droite
        if op == ">":
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche > droite
        if op == "<=":
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche <= droite
        if op == ">=":
            self._verifier_nombres(gauche, droite, op, ligne)
            return gauche >= droite

        raise ErreurExecution(f"operateur inconnu: {op}", ligne)

    def _verifier_nombres(self, gauche, droite, op, ligne):
        for valeur in (gauche, droite):
            if isinstance(valeur, bool) or not isinstance(valeur, (int, float)):
                raise ErreurExecution(
                    f"l'operateur '{op}' attend des nombres, recu {valeur!r}", ligne
                )

    def _eval_AppelFonction(self, noeud, env):
        arguments = [self._evaluer(arg, env) for arg in noeud.arguments]

        if noeud.nom in self.natives:
            arite, fonction = self.natives[noeud.nom]
            if len(arguments) != arite:
                raise ErreurExecution(
                    f"'{noeud.nom}' attend {arite} argument(s), recu {len(arguments)}", noeud.ligne
                )
            return fonction(*arguments, noeud.ligne)

        cible = env.obtenir(noeud.nom, noeud.ligne)
        if not isinstance(cible, FonctionEasyLang):
            raise ErreurExecution(f"'{noeud.nom}' n'est pas une fonction", noeud.ligne)
        return self._appeler_fonction(cible, arguments, noeud.ligne)

    def _eval_AccesMembre(self, noeud, env):
        cible = self._evaluer(noeud.cible, env)
        if not isinstance(cible, Module):
            raise ErreurExecution(f"impossible d'acceder au membre '{noeud.nom}': ce n'est pas un module", noeud.ligne)
        return cible.environnement.obtenir(noeud.nom, noeud.ligne)

    def _eval_AppelMethode(self, noeud, env):
        cible = self._evaluer(noeud.cible, env)
        if not isinstance(cible, Module):
            raise ErreurExecution(f"impossible d'appeler '{noeud.nom}': ce n'est pas un module", noeud.ligne)
        fonction = cible.environnement.obtenir(noeud.nom, noeud.ligne)
        if not isinstance(fonction, FonctionEasyLang):
            raise ErreurExecution(f"'{noeud.nom}' n'est pas une fonction exportee par le module '{cible.nom}'", noeud.ligne)
        arguments = [self._evaluer(arg, env) for arg in noeud.arguments]
        return self._appeler_fonction(fonction, arguments, noeud.ligne)

    def _appeler_fonction(self, fonction, arguments, ligne):
        if len(arguments) != len(fonction.parametres):
            raise ErreurExecution(
                f"la fonction '{fonction.nom}' attend {len(fonction.parametres)} argument(s), recu {len(arguments)}",
                ligne,
            )

        env_appel = Environnement(fonction.environnement_definition)
        for nom_param, valeur in zip(fonction.parametres, arguments):
            env_appel.definir(nom_param, valeur)

        try:
            self._executer_bloc(fonction.bloc, env_appel)
        except SignalRetour as retour:
            return retour.valeur
        return None
