#include "debogueur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "environnement.h"
#include "interpreteur.h"
#include "valeur.h"

#define MAX_POINTS_ARRET 256

static int points_arret[MAX_POINTS_ARRET];
static int nb_points_arret = 0;
static int mode_pas = 1;

static char **lignes_source;
static int nb_lignes_source;

static int est_point_arret(int ligne) {
    for (int i = 0; i < nb_points_arret; i++) {
        if (points_arret[i] == ligne) return 1;
    }
    return 0;
}

static void ajouter_point_arret(int ligne) {
    if (nb_points_arret < MAX_POINTS_ARRET) points_arret[nb_points_arret++] = ligne;
    printf("point d'arret ajoute a la ligne %d\n", ligne);
}

static void retirer_point_arret(int ligne) {
    for (int i = 0; i < nb_points_arret; i++) {
        if (points_arret[i] == ligne) {
            points_arret[i] = points_arret[--nb_points_arret];
            break;
        }
    }
    printf("point d'arret retire de la ligne %d\n", ligne);
}

static void afficher_variable(const char *nom, Environnement *env) {
    Valeur v;
    if (environnement_obtenir(env, nom, &v)) {
        char *s = valeur_formater(v);
        printf("%s = %s\n", nom, s);
        free(s);
    } else {
        printf("variable inconnue: '%s'\n", nom);
    }
}

static void afficher_variables(Environnement *env) {
    char *vus[256];
    int nb_vus = 0;
    for (Environnement *e = env; e != NULL; e = e->parent) {
        for (int i = 0; i < e->compte; i++) {
            int deja_vu = 0;
            for (int k = 0; k < nb_vus; k++) {
                if (strcmp(vus[k], e->noms[i]) == 0) { deja_vu = 1; break; }
            }
            if (deja_vu) continue;
            if (nb_vus < 256) vus[nb_vus++] = e->noms[i];
            char *s = valeur_formater(e->valeurs[i]);
            printf("%s = %s\n", e->noms[i], s);
            free(s);
        }
    }
}

static void afficher_aide(void) {
    printf(
        "commandes:\n"
        "  n, suivant       executer la ligne courante et s'arreter a la prochaine\n"
        "  c, continuer     continuer jusqu'au prochain point d'arret\n"
        "  ba <ligne>       ajouter un point d'arret\n"
        "  br <ligne>       retirer un point d'arret\n"
        "  b, arrets        lister les points d'arret\n"
        "  v, variables     afficher toutes les variables visibles\n"
        "  p <nom>          afficher la valeur d'une variable\n"
        "  q, quitter       arreter l'execution\n"
    );
}

static void boucle_commandes(Environnement *env) {
    char ligne[512];
    for (;;) {
        printf("(debogueur) ");
        fflush(stdout);
        if (!fgets(ligne, sizeof(ligne), stdin)) { mode_pas = 0; return; }

        size_t len = strlen(ligne);
        while (len > 0 && (ligne[len - 1] == '\n' || ligne[len - 1] == '\r')) ligne[--len] = '\0';

        if (len == 0 || strcmp(ligne, "n") == 0 || strcmp(ligne, "suivant") == 0) { mode_pas = 1; return; }
        if (strcmp(ligne, "c") == 0 || strcmp(ligne, "continuer") == 0) { mode_pas = 0; return; }
        if (strcmp(ligne, "q") == 0 || strcmp(ligne, "quitter") == 0) {
            printf("=== execution arretee par l'utilisateur ===\n");
            exit(0);
        }
        if (strcmp(ligne, "v") == 0 || strcmp(ligne, "variables") == 0) { afficher_variables(env); continue; }
        if (strcmp(ligne, "b") == 0 || strcmp(ligne, "arrets") == 0) {
            printf("points d'arret:");
            for (int i = 0; i < nb_points_arret; i++) printf(" %d", points_arret[i]);
            if (nb_points_arret == 0) printf(" aucun");
            printf("\n");
            continue;
        }
        if (strncmp(ligne, "ba ", 3) == 0) { ajouter_point_arret(atoi(ligne + 3)); continue; }
        if (strncmp(ligne, "br ", 3) == 0) { retirer_point_arret(atoi(ligne + 3)); continue; }
        if (strncmp(ligne, "p ", 2) == 0) { afficher_variable(ligne + 2, env); continue; }
        if (strncmp(ligne, "affiche ", 8) == 0) { afficher_variable(ligne + 8, env); continue; }
        if (strcmp(ligne, "aide") == 0 || strcmp(ligne, "?") == 0 || strcmp(ligne, "h") == 0) { afficher_aide(); continue; }

        printf("commande inconnue: '%s' (tapez 'aide')\n", ligne);
    }
}

static void hook_debogueur(Noeud *n, Environnement *env) {
    int ligne = n->ligne;
    if (!mode_pas && !est_point_arret(ligne)) return;

    const char *texte = (ligne >= 1 && ligne <= nb_lignes_source) ? lignes_source[ligne - 1] : "";
    printf("-> ligne %d: %s\n", ligne, texte);
    boucle_commandes(env);
}

void debogueur_installer(char **lignes, int nb_lignes, int *points, int nb_points) {
    lignes_source = lignes;
    nb_lignes_source = nb_lignes;
    for (int i = 0; i < nb_points && i < MAX_POINTS_ARRET; i++) points_arret[nb_points_arret++] = points[i];
    mode_pas = (nb_points == 0);
    interpreteur_definir_hook(hook_debogueur);
}
