#include "environnement.h"

#include <stdlib.h>
#include <string.h>

Environnement *environnement_creer(Environnement *parent) {
    Environnement *env = malloc(sizeof(Environnement));
    env->capacite = 8;
    env->compte = 0;
    env->noms = malloc(sizeof(char *) * env->capacite);
    env->valeurs = malloc(sizeof(Valeur) * env->capacite);
    env->parent = parent;
    return env;
}

static int trouver_local(Environnement *env, const char *nom) {
    for (int i = 0; i < env->compte; i++) {
        if (strcmp(env->noms[i], nom) == 0) return i;
    }
    return -1;
}

void environnement_definir(Environnement *env, const char *nom, Valeur v) {
    int idx = trouver_local(env, nom);
    if (idx >= 0) {
        env->valeurs[idx] = v;
        return;
    }
    if (env->compte >= env->capacite) {
        env->capacite *= 2;
        env->noms = realloc(env->noms, sizeof(char *) * env->capacite);
        env->valeurs = realloc(env->valeurs, sizeof(Valeur) * env->capacite);
    }
    env->noms[env->compte] = strdup(nom);
    env->valeurs[env->compte] = v;
    env->compte++;
}

int environnement_obtenir(Environnement *env, const char *nom, Valeur *dehors) {
    for (Environnement *e = env; e != NULL; e = e->parent) {
        int idx = trouver_local(e, nom);
        if (idx >= 0) {
            *dehors = e->valeurs[idx];
            return 1;
        }
    }
    return 0;
}

void environnement_detruire(Environnement *env) {
    for (int i = 0; i < env->compte; i++) free(env->noms[i]);
    free(env->noms);
    free(env->valeurs);
    free(env);
}

int environnement_assigner(Environnement *env, const char *nom, Valeur v) {
    for (Environnement *e = env; e != NULL; e = e->parent) {
        int idx = trouver_local(e, nom);
        if (idx >= 0) {
            e->valeurs[idx] = v;
            return 1;
        }
    }
    return 0;
}
