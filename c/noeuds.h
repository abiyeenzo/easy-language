#ifndef NOEUDS_H
#define NOEUDS_H

typedef struct Noeud Noeud;

typedef enum {
    N_PROGRAMME, N_DECLARATION, N_AFFECTATION, N_AFFICHE, N_DEMANDE,
    N_SI, N_TANTQUE, N_POUR, N_POUR_CHAQUE, N_FONCTION_DEF, N_RETOURNE,
    N_ARRETE, N_CONTINUE, N_EXPR_INSTRUCTION, N_AFFECTATION_INDEX,
    N_LITTERAL_ENTIER, N_LITTERAL_REEL, N_LITTERAL_TEXTE, N_LITTERAL_BOOLEEN, N_LITTERAL_RIEN,
    N_VARIABLE, N_BINAIRE, N_UNAIRE, N_APPEL, N_LISTE, N_INDEXATION,
    N_IMPORTATION, N_ACCES_MEMBRE, N_APPEL_METHODE,
} TypeNoeud;

typedef enum {
    OP_PLUS, OP_MOINS, OP_FOIS, OP_DIVISE, OP_MODULO,
    OP_EGAL, OP_DIFFERENT, OP_INF, OP_SUP, OP_INF_EGAL, OP_SUP_EGAL,
    OP_ET, OP_OU, OP_NON, OP_NEG,
} Operateur;

typedef struct {
    Noeud *condition;
    Noeud **bloc;
    int nb_bloc;
} Branche;

struct Noeud {
    TypeNoeud type;
    int ligne;

    /* litteraux */
    long long entier;
    double reel;
    char *texte;
    int booleen;

    /* variable / declaration / affectation / demande / appel / fonction_def / pour / pour_chaque
       importation (nom = alias, peut etre NULL ; texte = chemin) / acces_membre / appel_methode (nom = membre) */
    char *nom;
    char *invite; /* demande, peut etre NULL */

    /* expressions generiques */
    Noeud *gauche;
    Noeud *droite;
    Noeud *cible;
    Noeud *index;
    Noeud *expression;
    Operateur operateur;

    /* listes d'enfants (affiche, appel, liste litterale, arguments) */
    Noeud **enfants;
    int nb_enfants;

    /* si */
    Branche *branches;
    int nb_branches;
    Noeud **sinon;
    int nb_sinon;

    /* tantque / pour / pour_chaque / fonction_def */
    Noeud **bloc;
    int nb_bloc;

    /* pour */
    Noeud *depart;
    Noeud *fin;
    Noeud *pas;

    /* fonction_def */
    char **parametres;
    int nb_parametres;

    /* programme */
    Noeud **instructions;
    int nb_instructions;
};

#endif
