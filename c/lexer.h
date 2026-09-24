#ifndef LEXER_H
#define LEXER_H

typedef enum {
    T_SOIT, T_AFFICHE, T_DEMANDE,
    T_SI, T_ALORS, T_SINON,
    T_TANTQUE, T_POUR, T_DE, T_JUSQUA, T_PAS, T_CHAQUE, T_DANS,
    T_FONCTION, T_RETOURNE,
    T_VRAI, T_FAUX, T_RIEN,
    T_ET, T_OU, T_NON,
    T_ARRETE, T_CONTINUE,
    T_IMPORTE, T_COMME,
    T_IDENT, T_NOMBRE_ENTIER, T_NOMBRE_REEL, T_TEXTE,
    T_EGAL_EGAL, T_DIFFERENT, T_INF_EGAL, T_SUP_EGAL,
    T_PLUS, T_MOINS, T_FOIS, T_DIVISE, T_MODULO,
    T_EGAL, T_INF, T_SUP,
    T_PARO, T_PARF, T_CROCHET_O, T_CROCHET_F, T_DEUX_POINTS, T_VIRGULE, T_POINT,
    T_INDENT, T_DEDENT, T_NOUVELLE_LIGNE, T_FIN,
} TypeJeton;

typedef struct {
    TypeJeton type;
    char *texte;        /* pour T_IDENT / T_TEXTE */
    long long entier;   /* pour T_NOMBRE_ENTIER */
    double reel;         /* pour T_NOMBRE_REEL */
    int ligne;
    /* Y avait-il au moins une espace immediatement avant ce jeton (sur la
       meme ligne) ? Sert uniquement a lever l'ambiguite d'un '-' unaire
       colle a l'operande suivant apres un espace (voir analyseur.c,
       "3 -4" lu comme deux arguments plutot qu'une soustraction). */
    int espace_avant;
} Jeton;

typedef struct {
    Jeton *jetons;
    int compte;
    int capacite;
} ListeJetons;

/* Tokenise la source complete. Termine le programme avec un message
   d'erreur sur stderr en cas d'erreur lexicale (pas de gestion d'erreur
   recuperable ici, comme il se doit pour un petit interpreteur). */
ListeJetons lexer_tokeniser(const char *source);

#endif
