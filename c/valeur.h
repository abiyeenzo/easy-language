#ifndef VALEUR_H
#define VALEUR_H

#include <stdint.h>

typedef struct Noeud Noeud;
typedef struct Environnement Environnement;
typedef struct Valeur Valeur;

typedef enum {
    V_RIEN,
    V_ENTIER,
    V_REEL,
    V_BOOLEEN,
    V_TEXTE,
    V_LISTE,
    V_FONCTION,
    V_MODULE,
} TypeValeur;

typedef struct {
    Valeur *elements;
    int compte;
    int capacite;
} Liste;

typedef struct {
    char **parametres;
    int nb_parametres;
    Noeud **bloc;
    int nb_instructions;
    Environnement *env_definition;
    const char *nom;
} FonctionVal;

typedef struct {
    char *nom;
    Environnement *environnement;
} ModuleVal;

struct Valeur {
    TypeValeur type;
    union {
        int64_t entier;
        double reel;
        int booleen;
        char *texte;
        Liste *liste;
        FonctionVal *fonction;
        ModuleVal *module;
    } comme;
};

Valeur valeur_rien(void);
Valeur valeur_entier(int64_t v);
Valeur valeur_reel(double v);
Valeur valeur_booleen(int v);
Valeur valeur_texte(const char *v);
Valeur valeur_liste_vide(void);
Valeur valeur_fonction(FonctionVal *f);
Valeur valeur_module(ModuleVal *m);

int valeur_est_vraie(Valeur v);
int valeur_est_nombre(Valeur v);
double valeur_comme_reel(Valeur v);
int valeur_egales(Valeur a, Valeur b);

/* Retourne une chaine allouee (a liberer par l'appelant si besoin; ce projet
   ne libere jamais la memoire, voir README du dossier c/). */
char *valeur_formater(Valeur v);

void liste_ajoute(Liste *liste, Valeur v);

#endif
