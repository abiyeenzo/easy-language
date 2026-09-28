#ifndef ENVIRONNEMENT_H
#define ENVIRONNEMENT_H

#include "valeur.h"

typedef struct Environnement {
    char **noms;
    Valeur *valeurs;
    int compte;
    int capacite;
    struct Environnement *parent;
    int refs;
} Environnement;

Environnement *environnement_creer(Environnement *parent);
/* Termine l'usage normal (fin de bloc/boucle/appel) de cet environnement :
   decremente son compteur de references et ne le recycle reellement (pour
   un futur environnement_creer) que s'il n'est plus capture par aucune
   fermeture vivante (voir environnement_capturer). Si une fonction definie
   a l'interieur de ce bloc a ete conservee (retournee, assignee ailleurs,
   stockee dans une liste...), l'environnement reste vivant tant que cette
   fonction existe : les fermetures fonctionnent au-dela de la duree de vie
   de leur bloc. */
void environnement_detruire(Environnement *env);
/* A appeler exactement une fois par FonctionVal creee, sur l'environnement
   ou elle est definie (f->env_definition), pour que cet environnement (et
   toute sa chaine de parents) reste vivant tant que cette fonction existe.
   Les FonctionVal ne sont jamais liberees (comme le reste des valeurs de ce
   projet, cf. valeur.h), donc cette capture est permanente : coherent avec
   le fait qu'une fermeture reste appelable indefiniment. */
void environnement_capturer(Environnement *env);
void environnement_definir(Environnement *env, const char *nom, Valeur v);
/* Retourne 1 et remplit *dehors si trouve, sinon 0. */
int environnement_obtenir(Environnement *env, const char *nom, Valeur *dehors);
/* Retourne 1 si assigne (variable deja declaree quelque part dans la chaine), sinon 0. */
int environnement_assigner(Environnement *env, const char *nom, Valeur v);

/* Variantes avec cache de position (niveau, indice), a fournir par
   l'appelant et initialiser a -1 (pas encore resolu). Voir
   environnement.c pour le principe. A utiliser uniquement quand le meme
   couple (niveau, indice) sera reutilise a travers plusieurs appels pour
   le meme site dans le code (typiquement stocke sur un noeud d'AST) :
   sinon la recherche complete est refaite a chaque fois sans gain. */
int environnement_obtenir_cache(Environnement *env, const char *nom, int *niveau, int *indice, Valeur *dehors);
int environnement_assigner_cache(Environnement *env, const char *nom, int *niveau, int *indice, Valeur v);

#endif
