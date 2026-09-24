#include "analyseur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Jeton *jetons;
    int position;
} Analyseur;

static Noeud *expression(Analyseur *a);
static Noeud **bloc(Analyseur *a, int *nb_dehors);

static Noeud *creer_noeud(TypeNoeud type, int ligne) {
    Noeud *n = calloc(1, sizeof(Noeud));
    n->type = type;
    n->ligne = ligne;
    n->cache_natif = -1;
    n->cache_niveau = -1;
    n->cache_indice = -1;
    return n;
}

static Noeud **noeuds_ajouter(Noeud **tab, int *compte, int *capacite, Noeud *item) {
    if (*compte >= *capacite) {
        *capacite = *capacite ? *capacite * 2 : 4;
        tab = realloc(tab, sizeof(Noeud *) * (*capacite));
    }
    tab[(*compte)++] = item;
    return tab;
}

static char **chaines_ajouter(char **tab, int *compte, int *capacite, char *item) {
    if (*compte >= *capacite) {
        *capacite = *capacite ? *capacite * 2 : 4;
        tab = realloc(tab, sizeof(char *) * (*capacite));
    }
    tab[(*compte)++] = item;
    return tab;
}

static Jeton *actuel(Analyseur *a) { return &a->jetons[a->position]; }
static TypeJeton type_actuel(Analyseur *a) { return actuel(a)->type; }
static Jeton avancer(Analyseur *a) { return a->jetons[a->position++]; }
static int correspond(Analyseur *a, TypeJeton t) { return type_actuel(a) == t; }

static void erreur_syntaxe(int ligne, const char *msg) {
    fprintf(stderr, "Erreur ligne %d: %s\n", ligne, msg);
    exit(1);
}

static Jeton attendre(Analyseur *a, TypeJeton t, const char *msg) {
    if (type_actuel(a) != t) erreur_syntaxe(actuel(a)->ligne, msg);
    return avancer(a);
}

static void virgule_optionnelle(Analyseur *a) {
    if (correspond(a, T_VIRGULE)) avancer(a);
}

static int debut_expression(TypeJeton t) {
    switch (t) {
        case T_NOMBRE_ENTIER: case T_NOMBRE_REEL: case T_TEXTE: case T_IDENT:
        case T_PARO: case T_CROCHET_O: case T_VRAI: case T_FAUX: case T_RIEN:
        case T_NON: case T_MOINS:
            return 1;
        default:
            return 0;
    }
}

static void ignorer_lignes_vides(Analyseur *a) {
    while (correspond(a, T_NOUVELLE_LIGNE)) avancer(a);
}

static void fin_instruction(Analyseur *a) {
    if (correspond(a, T_FIN)) return;
    attendre(a, T_NOUVELLE_LIGNE, "fin de ligne attendue");
}

static Noeud *binaire(Noeud *g, Operateur op, Noeud *d, int ligne) {
    Noeud *n = creer_noeud(N_BINAIRE, ligne);
    n->gauche = g; n->operateur = op; n->droite = d;
    return n;
}

static Noeud *unaire(Operateur op, Noeud *operande, int ligne) {
    Noeud *n = creer_noeud(N_UNAIRE, ligne);
    n->operateur = op; n->expression = operande;
    return n;
}

/* --- expressions, precedence croissante --- */

static Noeud *primaire_base(Analyseur *a) {
    Jeton j = *actuel(a);

    if (j.type == T_CROCHET_O) {
        avancer(a);
        Noeud *n = creer_noeud(N_LISTE, j.ligne);
        int cap = 0;
        while (debut_expression(type_actuel(a))) {
            n->enfants = noeuds_ajouter(n->enfants, &n->nb_enfants, &cap, expression(a));
            virgule_optionnelle(a);
        }
        attendre(a, T_CROCHET_F, "']' attendu pour fermer la liste");
        return n;
    }
    if (j.type == T_NOMBRE_ENTIER) { avancer(a); Noeud *n = creer_noeud(N_LITTERAL_ENTIER, j.ligne); n->entier = j.entier; return n; }
    if (j.type == T_NOMBRE_REEL) { avancer(a); Noeud *n = creer_noeud(N_LITTERAL_REEL, j.ligne); n->reel = j.reel; return n; }
    if (j.type == T_TEXTE) { avancer(a); Noeud *n = creer_noeud(N_LITTERAL_TEXTE, j.ligne); n->texte = j.texte; return n; }
    if (j.type == T_VRAI) { avancer(a); Noeud *n = creer_noeud(N_LITTERAL_BOOLEEN, j.ligne); n->booleen = 1; return n; }
    if (j.type == T_FAUX) { avancer(a); Noeud *n = creer_noeud(N_LITTERAL_BOOLEEN, j.ligne); n->booleen = 0; return n; }
    if (j.type == T_RIEN) { avancer(a); return creer_noeud(N_LITTERAL_RIEN, j.ligne); }
    if (j.type == T_PARO) {
        avancer(a);
        Noeud *n = expression(a);
        attendre(a, T_PARF, "')' attendu");
        return n;
    }
    if (j.type == T_IDENT) {
        avancer(a);
        if (correspond(a, T_PARO)) {
            avancer(a);
            Noeud *n = creer_noeud(N_APPEL, j.ligne);
            n->nom = j.texte;
            int cap = 0;
            while (debut_expression(type_actuel(a))) {
                n->enfants = noeuds_ajouter(n->enfants, &n->nb_enfants, &cap, expression(a));
                virgule_optionnelle(a);
            }
            attendre(a, T_PARF, "')' attendu apres les arguments");
            return n;
        }
        Noeud *n = creer_noeud(N_VARIABLE, j.ligne);
        n->nom = j.texte;
        return n;
    }

    erreur_syntaxe(j.ligne, "expression inattendue");
    return NULL;
}

static Noeud *primaire(Analyseur *a) {
    Noeud *cible = primaire_base(a);
    if (cible->type == N_LISTE) return cible;
    for (;;) {
        /* Meme principe que pour le '-' unaire (voir addition()) : sans
           virgule dans les listes d'arguments, "f(x [1 2])" doit lire
           deux arguments (x, puis le litteral de liste [1 2]), pas
           l'indexation "x[1 2]". Un '[' colle a la cible (sans espace
           avant) reste de l'indexation (x[0]), comme toujours ; un '['
           precede d'une espace arrete plutot ce postfixe pour laisser
           l'appelant (liste d'arguments) le lire comme un nouvel
           element. Le '.' (acces de membre / module) n'a pas cette
           ambiguite : aucun autre sens valide n'existe pour lui a cette
           position, l'espacement n'y change donc rien. */
        if (correspond(a, T_CROCHET_O) && actuel(a)->espace_avant) break;
        if (!correspond(a, T_CROCHET_O) && !correspond(a, T_POINT)) break;

        if (correspond(a, T_CROCHET_O)) {
            int ligne = avancer(a).ligne;
            Noeud *idx = expression(a);
            attendre(a, T_CROCHET_F, "']' attendu");
            Noeud *n = creer_noeud(N_INDEXATION, ligne);
            n->cible = cible; n->index = idx;
            cible = n;
            continue;
        }

        int ligne = avancer(a).ligne;
        char *nom = attendre(a, T_IDENT, "nom de membre attendu apres '.'").texte;
        if (correspond(a, T_PARO)) {
            avancer(a);
            Noeud *n = creer_noeud(N_APPEL_METHODE, ligne);
            n->cible = cible; n->nom = nom;
            int cap = 0;
            while (debut_expression(type_actuel(a))) {
                n->enfants = noeuds_ajouter(n->enfants, &n->nb_enfants, &cap, expression(a));
                virgule_optionnelle(a);
            }
            attendre(a, T_PARF, "')' attendu apres les arguments");
            cible = n;
        } else {
            Noeud *n = creer_noeud(N_ACCES_MEMBRE, ligne);
            n->cible = cible; n->nom = nom;
            cible = n;
        }
    }
    return cible;
}

static Noeud *facteur(Analyseur *a) {
    if (correspond(a, T_MOINS)) {
        int ligne = avancer(a).ligne;
        return unaire(OP_NEG, facteur(a), ligne);
    }
    return primaire(a);
}

static Noeud *terme(Analyseur *a) {
    Noeud *g = facteur(a);
    for (;;) {
        Operateur op;
        if (correspond(a, T_FOIS)) op = OP_FOIS;
        else if (correspond(a, T_DIVISE)) op = OP_DIVISE;
        else if (correspond(a, T_MODULO)) op = OP_MODULO;
        else break;
        int ligne = avancer(a).ligne;
        g = binaire(g, op, facteur(a), ligne);
    }
    return g;
}

static Noeud *addition(Analyseur *a) {
    Noeud *g = terme(a);
    for (;;) {
        Operateur op;
        if (correspond(a, T_PLUS)) {
            op = OP_PLUS;
        } else if (correspond(a, T_MOINS)) {
            /* Sans virgule dans les listes d'arguments (affiche, appels,
               listes litterales), "3 -4" etait avale comme une seule
               soustraction "3 - 4" plutot que lu comme deux elements (3
               et -4). On distingue maintenant par l'espacement, comme
               Ruby le fait pour le meme probleme : un '-' precede d'une
               espace mais colle a l'operande suivant (aucune espace
               apres) marque le debut d'un nouvel element a la place
               d'une continuation de l'expression courante. "3 - 4"
               (espaces des deux cotes) et "3-4" (aucune espace) restent
               des soustractions, inchangees. */
            Jeton *moins = actuel(a);
            Jeton *apres = &a->jetons[a->position + 1];
            if (moins->espace_avant && !apres->espace_avant) break;
            op = OP_MOINS;
        } else {
            break;
        }
        int ligne = avancer(a).ligne;
        g = binaire(g, op, terme(a), ligne);
    }
    return g;
}

static Noeud *comparaison(Analyseur *a) {
    Noeud *g = addition(a);
    for (;;) {
        Operateur op;
        if (correspond(a, T_EGAL_EGAL)) op = OP_EGAL;
        else if (correspond(a, T_DIFFERENT)) op = OP_DIFFERENT;
        else if (correspond(a, T_INF)) op = OP_INF;
        else if (correspond(a, T_SUP)) op = OP_SUP;
        else if (correspond(a, T_INF_EGAL)) op = OP_INF_EGAL;
        else if (correspond(a, T_SUP_EGAL)) op = OP_SUP_EGAL;
        else break;
        int ligne = avancer(a).ligne;
        g = binaire(g, op, addition(a), ligne);
    }
    return g;
}

static Noeud *non_expr(Analyseur *a) {
    if (correspond(a, T_NON)) {
        int ligne = avancer(a).ligne;
        return unaire(OP_NON, non_expr(a), ligne);
    }
    return comparaison(a);
}

static Noeud *et_expr(Analyseur *a) {
    Noeud *g = non_expr(a);
    while (correspond(a, T_ET)) {
        int ligne = avancer(a).ligne;
        g = binaire(g, OP_ET, non_expr(a), ligne);
    }
    return g;
}

static Noeud *ou_expr(Analyseur *a) {
    Noeud *g = et_expr(a);
    while (correspond(a, T_OU)) {
        int ligne = avancer(a).ligne;
        g = binaire(g, OP_OU, et_expr(a), ligne);
    }
    return g;
}

static Noeud *expression(Analyseur *a) { return ou_expr(a); }

/* --- instructions --- */

static Noeud *instruction(Analyseur *a);

static Noeud **bloc(Analyseur *a, int *nb_dehors) {
    attendre(a, T_NOUVELLE_LIGNE, "un bloc doit commencer sur une nouvelle ligne");
    ignorer_lignes_vides(a);
    attendre(a, T_INDENT, "bloc indente attendu");
    Noeud **tab = NULL;
    int compte = 0, cap = 0;
    ignorer_lignes_vides(a);
    while (!correspond(a, T_DEDENT) && !correspond(a, T_FIN)) {
        tab = noeuds_ajouter(tab, &compte, &cap, instruction(a));
        ignorer_lignes_vides(a);
    }
    if (correspond(a, T_DEDENT)) avancer(a);
    *nb_dehors = compte;
    return tab;
}

static Noeud *declaration(Analyseur *a) {
    int ligne = avancer(a).ligne;
    char *nom = attendre(a, T_IDENT, "nom de variable attendu apres 'soit'").texte;
    attendre(a, T_EGAL, "'=' attendu");
    Noeud *expr = expression(a);
    fin_instruction(a);
    Noeud *n = creer_noeud(N_DECLARATION, ligne);
    n->nom = nom; n->expression = expr;
    return n;
}

static Noeud *expression_ou_affectation(Analyseur *a) {
    int ligne = actuel(a)->ligne;
    Noeud *cible = expression(a);
    if (correspond(a, T_EGAL)) {
        avancer(a);
        Noeud *valeur = expression(a);
        fin_instruction(a);
        if (cible->type == N_VARIABLE) {
            Noeud *n = creer_noeud(N_AFFECTATION, ligne);
            n->nom = cible->nom; n->expression = valeur;
            return n;
        }
        if (cible->type == N_INDEXATION) {
            Noeud *n = creer_noeud(N_AFFECTATION_INDEX, ligne);
            n->cible = cible->cible; n->index = cible->index; n->expression = valeur;
            return n;
        }
        erreur_syntaxe(ligne, "cible d'affectation invalide");
    }
    fin_instruction(a);
    Noeud *n = creer_noeud(N_EXPR_INSTRUCTION, ligne);
    n->expression = cible;
    return n;
}

static Noeud *affiche(Analyseur *a) {
    int ligne = avancer(a).ligne;
    Noeud *n = creer_noeud(N_AFFICHE, ligne);
    int cap = 0;
    n->enfants = noeuds_ajouter(n->enfants, &n->nb_enfants, &cap, expression(a));
    while (debut_expression(type_actuel(a))) {
        n->enfants = noeuds_ajouter(n->enfants, &n->nb_enfants, &cap, expression(a));
    }
    fin_instruction(a);
    return n;
}

static Noeud *demande(Analyseur *a) {
    int ligne = avancer(a).ligne;
    int parenthese = correspond(a, T_PARO);
    if (parenthese) avancer(a);
    char *nom = attendre(a, T_IDENT, "nom de variable attendu apres 'demande'").texte;
    char *invite = NULL;
    if (parenthese) virgule_optionnelle(a);
    if (correspond(a, T_TEXTE)) invite = avancer(a).texte;
    if (parenthese) attendre(a, T_PARF, "')' attendu apres 'demande'");
    fin_instruction(a);
    Noeud *n = creer_noeud(N_DEMANDE, ligne);
    n->nom = nom; n->invite = invite;
    return n;
}

static Noeud *si(Analyseur *a) {
    int ligne = avancer(a).ligne;
    Noeud *n = creer_noeud(N_SI, ligne);
    Noeud *cond = expression(a);
    attendre(a, T_ALORS, "'alors' attendu apres la condition");
    attendre(a, T_DEUX_POINTS, "':' attendu apres 'alors'");
    int nbb; Noeud **b = bloc(a, &nbb);
    n->branches = realloc(n->branches, sizeof(Branche) * (n->nb_branches + 1));
    n->branches[n->nb_branches].condition = cond;
    n->branches[n->nb_branches].bloc = b;
    n->branches[n->nb_branches].nb_bloc = nbb;
    n->nb_branches++;

    while (correspond(a, T_SINON)) {
        avancer(a);
        if (correspond(a, T_SI)) {
            avancer(a);
            Noeud *c2 = expression(a);
            attendre(a, T_ALORS, "'alors' attendu apres la condition");
            attendre(a, T_DEUX_POINTS, "':' attendu");
            int nbb2; Noeud **b2 = bloc(a, &nbb2);
            n->branches = realloc(n->branches, sizeof(Branche) * (n->nb_branches + 1));
            n->branches[n->nb_branches].condition = c2;
            n->branches[n->nb_branches].bloc = b2;
            n->branches[n->nb_branches].nb_bloc = nbb2;
            n->nb_branches++;
        } else {
            attendre(a, T_DEUX_POINTS, "':' attendu apres 'sinon'");
            n->sinon = bloc(a, &n->nb_sinon);
            break;
        }
    }
    return n;
}

static Noeud *tantque(Analyseur *a) {
    int ligne = avancer(a).ligne;
    Noeud *cond = expression(a);
    attendre(a, T_ALORS, "'alors' attendu apres la condition");
    attendre(a, T_DEUX_POINTS, "':' attendu");
    Noeud *n = creer_noeud(N_TANTQUE, ligne);
    n->expression = cond;
    n->bloc = bloc(a, &n->nb_bloc);
    return n;
}

static Noeud *pour(Analyseur *a) {
    int ligne = avancer(a).ligne;

    if (correspond(a, T_CHAQUE)) {
        avancer(a);
        char *variable = attendre(a, T_IDENT, "nom de variable attendu apres 'chaque'").texte;
        attendre(a, T_DANS, "'dans' attendu");
        Noeud *iterable = expression(a);
        attendre(a, T_ALORS, "'alors' attendu");
        attendre(a, T_DEUX_POINTS, "':' attendu");
        Noeud *n = creer_noeud(N_POUR_CHAQUE, ligne);
        n->nom = variable; n->expression = iterable;
        n->bloc = bloc(a, &n->nb_bloc);
        return n;
    }

    char *variable = attendre(a, T_IDENT, "nom de variable attendu apres 'pour'").texte;
    attendre(a, T_DE, "'de' attendu");
    Noeud *depart = expression(a);
    attendre(a, T_JUSQUA, "'jusqua' attendu");
    Noeud *fin = expression(a);
    Noeud *pas = NULL;
    if (correspond(a, T_PAS)) { avancer(a); pas = expression(a); }
    attendre(a, T_ALORS, "'alors' attendu");
    attendre(a, T_DEUX_POINTS, "':' attendu");
    Noeud *n = creer_noeud(N_POUR, ligne);
    n->nom = variable; n->depart = depart; n->fin = fin; n->pas = pas;
    n->bloc = bloc(a, &n->nb_bloc);
    return n;
}

static Noeud *definition_fonction(Analyseur *a) {
    int ligne = avancer(a).ligne;
    char *nom = attendre(a, T_IDENT, "nom de fonction attendu").texte;
    attendre(a, T_PARO, "'(' attendu apres le nom de la fonction");
    Noeud *n = creer_noeud(N_FONCTION_DEF, ligne);
    int cap = 0;
    while (correspond(a, T_IDENT)) {
        n->parametres = chaines_ajouter(n->parametres, &n->nb_parametres, &cap, avancer(a).texte);
        virgule_optionnelle(a);
    }
    attendre(a, T_PARF, "')' attendu");
    attendre(a, T_DEUX_POINTS, "':' attendu");
    n->nom = nom;
    n->bloc = bloc(a, &n->nb_bloc);
    return n;
}

static Noeud *retourne(Analyseur *a) {
    int ligne = avancer(a).ligne;
    Noeud *n = creer_noeud(N_RETOURNE, ligne);
    if (!correspond(a, T_NOUVELLE_LIGNE) && !correspond(a, T_FIN)) {
        n->expression = expression(a);
    }
    fin_instruction(a);
    return n;
}

static Noeud *importation(Analyseur *a) {
    int ligne = avancer(a).ligne;

    /* "importe nom" (identifiant nu, sans guillemets) designe un module de
       la bibliotheque standard fournie avec l'executable ; "importe
       "chemin.elg"" reste le mecanisme general pour importer le propre
       fichier .elg de l'utilisateur, resolu relativement au script en
       cours. Les deux partagent le reste de la syntaxe (comme alias). */
    int systeme = correspond(a, T_IDENT);
    char *chemin = systeme
        ? avancer(a).texte
        : attendre(a, T_TEXTE, "chemin de fichier (texte) ou nom de bibliotheque standard attendu apres 'importe'").texte;

    char *alias = NULL;
    if (correspond(a, T_COMME)) {
        avancer(a);
        alias = attendre(a, T_IDENT, "nom d'alias attendu apres 'comme'").texte;
    }
    fin_instruction(a);
    Noeud *n = creer_noeud(N_IMPORTATION, ligne);
    n->texte = chemin; n->nom = alias;
    n->importation_systeme = systeme;
    return n;
}

static Noeud *instruction(Analyseur *a) {
    switch (type_actuel(a)) {
        case T_SOIT: return declaration(a);
        case T_IMPORTE: return importation(a);
        case T_AFFICHE: return affiche(a);
        case T_DEMANDE: return demande(a);
        case T_SI: return si(a);
        case T_TANTQUE: return tantque(a);
        case T_POUR: return pour(a);
        case T_FONCTION: return definition_fonction(a);
        case T_RETOURNE: return retourne(a);
        case T_ARRETE: { int ligne = avancer(a).ligne; fin_instruction(a); return creer_noeud(N_ARRETE, ligne); }
        case T_CONTINUE: { int ligne = avancer(a).ligne; fin_instruction(a); return creer_noeud(N_CONTINUE, ligne); }
        default: return expression_ou_affectation(a);
    }
}

Noeud *analyseur_analyser(ListeJetons jetons) {
    Analyseur a = { jetons.jetons, 0 };
    Noeud *programme = creer_noeud(N_PROGRAMME, 1);
    int cap = 0;
    ignorer_lignes_vides(&a);
    while (!correspond(&a, T_FIN)) {
        programme->instructions = noeuds_ajouter(programme->instructions, &programme->nb_instructions, &cap, instruction(&a));
        ignorer_lignes_vides(&a);
    }
    return programme;
}
