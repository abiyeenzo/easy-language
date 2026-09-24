#include "environnement.h"

#include <stdlib.h>
#include <string.h>

/* Pile de reutilisation des Environnement de portee temporaire (bloc,
   boucle, appel de fonction). Ces portees sont creees et detruites en
   LIFO strict (jamais de fermeture qui survit a son bloc, cf.
   environnement.h), donc un simple free-list suffit : au lieu de
   malloc/free le struct et ses deux tableaux internes a chaque appel
   (jusqu'a 4 allocations par appel de fonction sur un million d'appels
   en recursion, ce qui dominait le temps d'execution en appels systeme
   malloc/free), on recycle l'Environnement libere le plus recemment,
   tableaux internes compris : leur capacite est deja bonne pour un
   appel de forme similaire, donc la reutilisation evite aussi le
   realloc. La pile grandit au besoin et n'est jamais liberee (comme le
   reste de la memoire du programme, cf. valeur.h). */
static Environnement **g_pile = NULL;
static int g_pile_compte = 0;
static int g_pile_capacite = 0;

Environnement *environnement_creer(Environnement *parent) {
    Environnement *env;
    if (g_pile_compte > 0) {
        env = g_pile[--g_pile_compte];
        env->compte = 0;
    } else {
        env = malloc(sizeof(Environnement));
        env->capacite = 8;
        env->compte = 0;
        env->noms = malloc(sizeof(char *) * env->capacite);
        env->valeurs = malloc(sizeof(Valeur) * env->capacite);
    }
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
    /* nom vient toujours d'un noeud de l'AST ou d'un FonctionVal, tous
       deux vivants pour toute la duree du programme (jamais liberes,
       cf. valeur.h) : pas besoin de dupliquer la chaine, on emprunte le
       pointeur. Le cast enleve juste le const, la chaine n'est jamais
       modifiee via env->noms. */
    env->noms[env->compte] = (char *)nom;
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

/* Marche comme un pas de la chaine de portees jusqu'a *niveau parents,
   puis verifie que la variable attendue est toujours a *indice a cet
   endroit (un seul strcmp). Retourne l'environnement trouve, ou NULL si
   le cache n'est plus valide (jamais resolu, portee trop courte, ou nom
   different a cette position). */
static Environnement *suivre_cache(Environnement *env, const char *nom, int niveau, int indice) {
    if (niveau < 0) return NULL;
    Environnement *e = env;
    for (int k = niveau; k > 0 && e; k--) e = e->parent;
    if (e && indice < e->compte && strcmp(e->noms[indice], nom) == 0) return e;
    return NULL;
}

/* Equivalent de environnement_obtenir, mais se souvient (via *niveau et
   *indice, fournis par l'appelant et geres par un noeud d'AST donne) de
   l'endroit ou la variable a ete trouvee la derniere fois. La position
   d'une variable pour un noeud donne ne depend que de la structure du
   code (jamais de l'etat d'execution), donc apres une premiere
   resolution complete, les appels suivants n'ont plus qu'a suivre
   *niveau portees parentes et comparer un seul nom, au lieu de reparcourir
   toute la chaine avec une comparaison par variable a chaque niveau. Ceci
   compte enormement sur du code recursif : la meme variable (ex: le nom
   d'une fonction qui s'appelle elle-meme) est alors resolue des millions
   de fois pour le meme noeud d'appel. */
int environnement_obtenir_cache(Environnement *env, const char *nom, int *niveau, int *indice, Valeur *dehors) {
    Environnement *e = suivre_cache(env, nom, *niveau, *indice);
    if (e) {
        *dehors = e->valeurs[*indice];
        return 1;
    }
    int n = 0;
    for (e = env; e != NULL; e = e->parent, n++) {
        int idx = trouver_local(e, nom);
        if (idx >= 0) {
            *dehors = e->valeurs[idx];
            *niveau = n;
            *indice = idx;
            return 1;
        }
    }
    return 0;
}

void environnement_detruire(Environnement *env) {
    /* les noms ne sont plus dupliques (voir environnement_definir), donc
       rien a liberer a part remettre l'environnement dans la pile de
       reutilisation pour le prochain environnement_creer */
    env->compte = 0;
    if (g_pile_compte >= g_pile_capacite) {
        g_pile_capacite = g_pile_capacite ? g_pile_capacite * 2 : 64;
        g_pile = realloc(g_pile, sizeof(Environnement *) * g_pile_capacite);
    }
    g_pile[g_pile_compte++] = env;
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

/* Equivalent cache de environnement_assigner, meme principe que
   environnement_obtenir_cache. */
int environnement_assigner_cache(Environnement *env, const char *nom, int *niveau, int *indice, Valeur v) {
    Environnement *e = suivre_cache(env, nom, *niveau, *indice);
    if (e) {
        e->valeurs[*indice] = v;
        return 1;
    }
    int n = 0;
    for (e = env; e != NULL; e = e->parent, n++) {
        int idx = trouver_local(e, nom);
        if (idx >= 0) {
            e->valeurs[idx] = v;
            *niveau = n;
            *indice = idx;
            return 1;
        }
    }
    return 0;
}
