#ifndef ENVIRONNEMENT_H
#define ENVIRONNEMENT_H

#include "valeur.h"

typedef struct Environnement {
    char **noms;
    Valeur *valeurs;
    int compte;
    int capacite;
    struct Environnement *parent;
} Environnement;

Environnement *environnement_creer(Environnement *parent);
/* Termine cet environnement (les liaisons qu'il contient, pas les valeurs
   qu'elles referencent : listes/textes/fonctions vivent dans leurs propres
   blocs et peuvent avoir survecu via une valeur de retour ou une affectation
   externe) et le recycle pour un futur environnement_creer plutot que de le
   liberer immediatement (voir environnement.c). A n'appeler que sur un
   environnement de portee temporaire (bloc, boucle, appel de fonction) dont
   on est sur qu'aucune fonction definie a l'interieur n'est appelee apres
   coup (pas de fermeture qui s'echappe). */
void environnement_detruire(Environnement *env);
void environnement_definir(Environnement *env, const char *nom, Valeur v);
/* Retourne 1 et remplit *dehors si trouve, sinon 0. */
int environnement_obtenir(Environnement *env, const char *nom, Valeur *dehors);
/* Retourne 1 si assigne (variable deja declaree quelque part dans la chaine), sinon 0. */
int environnement_assigner(Environnement *env, const char *nom, Valeur v);

#endif
