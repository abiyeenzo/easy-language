#include "interpreteur.h"

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analyseur.h"
#include "lexer.h"
#include "natifs_gui.h"
#include "natifs_math.h"
#include "natifs_os.h"
#include "natifs_reseau.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Point chaud: operations arithmetiques entieres en assembleur x86-64 */
/* inline (GNU C). Sur des operations aussi simples, un compilateur   */
/* -O2 genere deja le meme code machine ; l'interet ici est de montrer */
/* une integration ASM reelle sur le chemin le plus emprunte de       */
/* l'evaluateur (l'arithmetique entiere), pas de gagner sur ce cas     */
/* precis. Necessite x86-64 (Linux ou mingw-w64 pour Windows).        */
/* ------------------------------------------------------------------ */

static inline int64_t asm_add_i64(int64_t a, int64_t b) {
    int64_t resultat;
    __asm__ (
        "movq %1, %0\n\t"
        "addq %2, %0"
        : "=r" (resultat)
        : "r" (a), "r" (b)
    );
    return resultat;
}

static inline int64_t asm_sub_i64(int64_t a, int64_t b) {
    int64_t resultat;
    __asm__ (
        "movq %1, %0\n\t"
        "subq %2, %0"
        : "=r" (resultat)
        : "r" (a), "r" (b)
    );
    return resultat;
}

static inline int64_t asm_mul_i64(int64_t a, int64_t b) {
    int64_t resultat;
    __asm__ (
        "movq %1, %0\n\t"
        "imulq %2, %0"
        : "=r" (resultat)
        : "r" (a), "r" (b)
    );
    return resultat;
}

static inline int64_t asm_neg_i64(int64_t a) {
    int64_t resultat;
    __asm__ (
        "movq %1, %0\n\t"
        "negq %0"
        : "=r" (resultat)
        : "r" (a)
    );
    return resultat;
}

/* ------------------------------------------------------------------ */

typedef enum { STATUT_NORMAL, STATUT_RETOURNE, STATUT_ARRETE, STATUT_CONTINUE } Statut;

typedef struct {
    Statut statut;
    Valeur valeur;
} Resultat;

static Resultat executer_bloc(Noeud **instructions, int n, Environnement *env);
static Valeur evaluer(Noeud *expr, Environnement *env);

static void erreur_execution(int ligne, const char *msg) {
    fprintf(stderr, "Erreur ligne %d: %s\n", ligne, msg);
    exit(1);
}

static Resultat resultat_normal(void) {
    Resultat r; r.statut = STATUT_NORMAL; r.valeur = valeur_rien(); return r;
}

/* ------------------------------------------------------------------ */
/* modules (importe ... comme ...)                                     */

#define MAX_MODULES_CHARGES 64
#define MAX_IMPORTS_EN_COURS 64

typedef struct { char *chemin; Valeur module; } EntreeCacheModule;

static char g_dossier_actuel[4096] = ".";
static EntreeCacheModule g_cache_modules[MAX_MODULES_CHARGES];
static int g_nb_modules_charges = 0;
static char *g_modules_en_cours[MAX_IMPORTS_EN_COURS];
static int g_nb_modules_en_cours = 0;

void interpreteur_definir_dossier(const char *dossier) {
    snprintf(g_dossier_actuel, sizeof(g_dossier_actuel), "%s", dossier);
}

/* snprintf tronque proprement (toujours termine par '\0') si un chemin
   depasse 4096 caracteres ; GCC ne peut pas le prouver statiquement d'ou
   l'avertissement desactive ici localement. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
static void joindre_chemin(char *dehors, size_t taille, const char *dossier, const char *relatif) {
    int absolu = relatif[0] == '/' || relatif[0] == '\\' ||
                 (isalpha((unsigned char)relatif[0]) && relatif[1] == ':');
    if (absolu) snprintf(dehors, taille, "%s", relatif);
    else snprintf(dehors, taille, "%s/%s", dossier, relatif);
}
#pragma GCC diagnostic pop

static int chercher_module(const char *chemin, Valeur *dehors) {
    for (int i = 0; i < g_nb_modules_charges; i++) {
        if (strcmp(g_cache_modules[i].chemin, chemin) == 0) { *dehors = g_cache_modules[i].module; return 1; }
    }
    return 0;
}

static void enregistrer_module(const char *chemin, Valeur module) {
    if (g_nb_modules_charges < MAX_MODULES_CHARGES) {
        g_cache_modules[g_nb_modules_charges].chemin = strdup(chemin);
        g_cache_modules[g_nb_modules_charges].module = module;
        g_nb_modules_charges++;
    }
}

static int module_en_cours_de_chargement(const char *chemin) {
    for (int i = 0; i < g_nb_modules_en_cours; i++) {
        if (strcmp(g_modules_en_cours[i], chemin) == 0) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* fonctions utilisateur                                              */

static Valeur appeler_fonction(FonctionVal *f, Valeur *args, int nb_args, int ligne) {
    if (nb_args != f->nb_parametres) {
        char msg[128];
        snprintf(msg, sizeof(msg), "la fonction '%s' attend %d argument(s), recu %d", f->nom, f->nb_parametres, nb_args);
        erreur_execution(ligne, msg);
    }
    Environnement *env_appel = environnement_creer(f->env_definition);
    for (int i = 0; i < nb_args; i++) {
        environnement_definir(env_appel, f->parametres[i], args[i]);
    }
    Resultat r = executer_bloc(f->bloc, f->nb_instructions, env_appel);
    environnement_detruire(env_appel);
    if (r.statut == STATUT_RETOURNE) return r.valeur;
    return valeur_rien();
}

/* ------------------------------------------------------------------ */
/* fonctions natives                                                  */

static int indice_reel(double index_d, int taille, int ligne) {
    long long index = (long long)index_d;
    if (index < 0) index += taille;
    if (index < 0 || index >= taille) erreur_execution(ligne, "index hors limites");
    return (int)index;
}

static Valeur appeler_native(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "longueur") == 0) {
        if (nb_args != 1) erreur_execution(ligne, "'longueur' attend 1 argument");
        Valeur v = args[0];
        if (v.type == V_TEXTE) return valeur_entier((int64_t)strlen(v.comme.texte));
        if (v.type == V_LISTE) return valeur_entier(v.comme.liste->compte);
        erreur_execution(ligne, "longueur() ne s'applique pas a cette valeur");
    }
    if (strcmp(nom, "entier") == 0) {
        if (nb_args != 1) erreur_execution(ligne, "'entier' attend 1 argument");
        Valeur v = args[0];
        if (v.type == V_ENTIER) return v;
        if (v.type == V_REEL) return valeur_entier((int64_t)v.comme.reel);
        if (v.type == V_TEXTE) return valeur_entier(atoll(v.comme.texte));
        erreur_execution(ligne, "impossible de convertir en entier");
    }
    if (strcmp(nom, "nombre") == 0) {
        if (nb_args != 1) erreur_execution(ligne, "'nombre' attend 1 argument");
        Valeur v = args[0];
        if (valeur_est_nombre(v)) return v;
        if (v.type == V_TEXTE) {
            if (strchr(v.comme.texte, '.')) return valeur_reel(atof(v.comme.texte));
            return valeur_entier(atoll(v.comme.texte));
        }
        erreur_execution(ligne, "impossible de convertir en nombre");
    }
    if (strcmp(nom, "texte") == 0) {
        if (nb_args != 1) erreur_execution(ligne, "'texte' attend 1 argument");
        return valeur_texte(valeur_formater(args[0]));
    }
    if (strcmp(nom, "ajoute") == 0) {
        if (nb_args != 2) erreur_execution(ligne, "'ajoute' attend 2 arguments");
        if (args[0].type != V_LISTE) erreur_execution(ligne, "ajoute() attend une liste");
        liste_ajoute(args[0].comme.liste, args[1]);
        return valeur_rien();
    }
    if (strcmp(nom, "retire") == 0) {
        if (nb_args != 2) erreur_execution(ligne, "'retire' attend 2 arguments");
        if (args[0].type != V_LISTE) erreur_execution(ligne, "retire() attend une liste");
        if (!valeur_est_nombre(args[1])) erreur_execution(ligne, "retire() attend un index entier");
        Liste *l = args[0].comme.liste;
        int idx = indice_reel(valeur_comme_reel(args[1]), l->compte, ligne);
        Valeur retiree = l->elements[idx];
        for (int i = idx; i < l->compte - 1; i++) l->elements[i] = l->elements[i + 1];
        l->compte--;
        return retiree;
    }
    if (strcmp(nom, "contient") == 0) {
        if (nb_args != 2) erreur_execution(ligne, "'contient' attend 2 arguments");
        if (args[0].type == V_LISTE) {
            Liste *l = args[0].comme.liste;
            for (int i = 0; i < l->compte; i++) {
                if (valeur_egales(l->elements[i], args[1])) return valeur_booleen(1);
            }
            return valeur_booleen(0);
        }
        if (args[0].type == V_TEXTE && args[1].type == V_TEXTE) {
            return valeur_booleen(strstr(args[0].comme.texte, args[1].comme.texte) != NULL);
        }
        erreur_execution(ligne, "contient() attend une liste ou un texte");
    }
    return valeur_rien(); /* jamais atteint si nom est bien un natif connu */
}

static int est_native(const char *nom) {
    static const char *noms[] = {"longueur", "entier", "nombre", "texte", "ajoute", "retire", "contient"};
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* evaluation d'expressions                                           */

static void verifier_nombres(Valeur a, Valeur b, const char *op, int ligne) {
    if (!valeur_est_nombre(a) || !valeur_est_nombre(b)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "l'operateur '%s' attend des nombres", op);
        erreur_execution(ligne, msg);
    }
}

static Valeur evaluer_binaire(Noeud *n, Environnement *env) {
    if (n->operateur == OP_ET) {
        Valeur g = evaluer(n->gauche, env);
        if (!valeur_est_vraie(g)) return g;
        return evaluer(n->droite, env);
    }
    if (n->operateur == OP_OU) {
        Valeur g = evaluer(n->gauche, env);
        if (valeur_est_vraie(g)) return g;
        return evaluer(n->droite, env);
    }

    Valeur g = evaluer(n->gauche, env);
    Valeur d = evaluer(n->droite, env);

    switch (n->operateur) {
        case OP_PLUS:
            if (g.type == V_LISTE && d.type == V_LISTE) {
                Valeur resultat = valeur_liste_vide();
                for (int i = 0; i < g.comme.liste->compte; i++) liste_ajoute(resultat.comme.liste, g.comme.liste->elements[i]);
                for (int i = 0; i < d.comme.liste->compte; i++) liste_ajoute(resultat.comme.liste, d.comme.liste->elements[i]);
                return resultat;
            }
            if (g.type == V_TEXTE || d.type == V_TEXTE) {
                char *sg = valeur_formater(g), *sd = valeur_formater(d);
                char *r = malloc(strlen(sg) + strlen(sd) + 1);
                strcpy(r, sg); strcat(r, sd);
                Valeur v = valeur_texte(r);
                free(sg); free(sd); free(r);
                return v;
            }
            verifier_nombres(g, d, "+", n->ligne);
            if (g.type == V_ENTIER && d.type == V_ENTIER) return valeur_entier(asm_add_i64(g.comme.entier, d.comme.entier));
            return valeur_reel(valeur_comme_reel(g) + valeur_comme_reel(d));

        case OP_MOINS:
            verifier_nombres(g, d, "-", n->ligne);
            if (g.type == V_ENTIER && d.type == V_ENTIER) return valeur_entier(asm_sub_i64(g.comme.entier, d.comme.entier));
            return valeur_reel(valeur_comme_reel(g) - valeur_comme_reel(d));

        case OP_FOIS:
            if (g.type == V_TEXTE && d.type == V_ENTIER) {
                int64_t fois = d.comme.entier;
                size_t lg = strlen(g.comme.texte);
                char *r = malloc(lg * (fois > 0 ? fois : 0) + 1);
                r[0] = '\0';
                for (int64_t i = 0; i < fois; i++) strcat(r, g.comme.texte);
                Valeur v = valeur_texte(r);
                free(r);
                return v;
            }
            verifier_nombres(g, d, "*", n->ligne);
            if (g.type == V_ENTIER && d.type == V_ENTIER) return valeur_entier(asm_mul_i64(g.comme.entier, d.comme.entier));
            return valeur_reel(valeur_comme_reel(g) * valeur_comme_reel(d));

        case OP_DIVISE:
            verifier_nombres(g, d, "/", n->ligne);
            if (valeur_comme_reel(d) == 0.0) erreur_execution(n->ligne, "division par zero");
            return valeur_reel(valeur_comme_reel(g) / valeur_comme_reel(d));

        case OP_MODULO:
            verifier_nombres(g, d, "%", n->ligne);
            if (g.type == V_ENTIER && d.type == V_ENTIER) {
                if (d.comme.entier == 0) erreur_execution(n->ligne, "division par zero");
                long long r = g.comme.entier % d.comme.entier;
                if ((r != 0) && ((r < 0) != (d.comme.entier < 0))) r += d.comme.entier;
                return valeur_entier(r);
            }
            if (valeur_comme_reel(d) == 0.0) erreur_execution(n->ligne, "division par zero");
            return valeur_reel(fmod(valeur_comme_reel(g), valeur_comme_reel(d)));

        case OP_EGAL: return valeur_booleen(valeur_egales(g, d));
        case OP_DIFFERENT: return valeur_booleen(!valeur_egales(g, d));
        case OP_INF: verifier_nombres(g, d, "<", n->ligne); return valeur_booleen(valeur_comme_reel(g) < valeur_comme_reel(d));
        case OP_SUP: verifier_nombres(g, d, ">", n->ligne); return valeur_booleen(valeur_comme_reel(g) > valeur_comme_reel(d));
        case OP_INF_EGAL: verifier_nombres(g, d, "<=", n->ligne); return valeur_booleen(valeur_comme_reel(g) <= valeur_comme_reel(d));
        case OP_SUP_EGAL: verifier_nombres(g, d, ">=", n->ligne); return valeur_booleen(valeur_comme_reel(g) >= valeur_comme_reel(d));
        default: break;
    }
    erreur_execution(n->ligne, "operateur binaire inconnu");
    return valeur_rien();
}

static Valeur evaluer(Noeud *n, Environnement *env) {
    switch (n->type) {
        case N_LITTERAL_ENTIER: return valeur_entier(n->entier);
        case N_LITTERAL_REEL: return valeur_reel(n->reel);
        case N_LITTERAL_TEXTE: return valeur_texte(n->texte);
        case N_LITTERAL_BOOLEEN: return valeur_booleen(n->booleen);
        case N_LITTERAL_RIEN: return valeur_rien();

        case N_VARIABLE: {
            Valeur v;
            if (!environnement_obtenir(env, n->nom, &v)) {
                char msg[160];
                snprintf(msg, sizeof(msg), "variable inconnue: '%s'", n->nom);
                erreur_execution(n->ligne, msg);
            }
            return v;
        }

        case N_UNAIRE: {
            Valeur v = evaluer(n->expression, env);
            if (n->operateur == OP_NEG) {
                if (!valeur_est_nombre(v)) erreur_execution(n->ligne, "impossible d'appliquer '-' a cette valeur");
                if (v.type == V_ENTIER) return valeur_entier(asm_neg_i64(v.comme.entier));
                return valeur_reel(-v.comme.reel);
            }
            return valeur_booleen(!valeur_est_vraie(v));
        }

        case N_BINAIRE: return evaluer_binaire(n, env);

        case N_LISTE: {
            Valeur v = valeur_liste_vide();
            for (int i = 0; i < n->nb_enfants; i++) liste_ajoute(v.comme.liste, evaluer(n->enfants[i], env));
            return v;
        }

        case N_INDEXATION: {
            Valeur conteneur = evaluer(n->cible, env);
            Valeur index = evaluer(n->index, env);
            if (!valeur_est_nombre(index)) erreur_execution(n->ligne, "index invalide");
            if (conteneur.type == V_LISTE) {
                int idx = indice_reel(valeur_comme_reel(index), conteneur.comme.liste->compte, n->ligne);
                return conteneur.comme.liste->elements[idx];
            }
            if (conteneur.type == V_TEXTE) {
                int taille = (int)strlen(conteneur.comme.texte);
                int idx = indice_reel(valeur_comme_reel(index), taille, n->ligne);
                char car[2] = { conteneur.comme.texte[idx], '\0' };
                return valeur_texte(car);
            }
            erreur_execution(n->ligne, "impossible d'indexer cette valeur");
            return valeur_rien();
        }

        case N_APPEL: {
            Valeur args[64];
            int nb = n->nb_enfants;
            if (nb > 64) erreur_execution(n->ligne, "trop d'arguments");
            for (int i = 0; i < nb; i++) args[i] = evaluer(n->enfants[i], env);

            if (est_native(n->nom)) return appeler_native(n->nom, args, nb, n->ligne);
            if (natifs_math_est(n->nom)) return natifs_math_appeler(n->nom, args, nb, n->ligne);
            if (natifs_os_est(n->nom)) return natifs_os_appeler(n->nom, args, nb, n->ligne);
            if (natifs_reseau_est(n->nom)) return natifs_reseau_appeler(n->nom, args, nb, n->ligne);
            if (natifs_gui_est(n->nom)) return natifs_gui_appeler(n->nom, args, nb, n->ligne);

            Valeur cible;
            if (!environnement_obtenir(env, n->nom, &cible)) {
                char msg[160];
                snprintf(msg, sizeof(msg), "variable inconnue: '%s'", n->nom);
                erreur_execution(n->ligne, msg);
            }
            if (cible.type != V_FONCTION) {
                char msg[160];
                snprintf(msg, sizeof(msg), "'%s' n'est pas une fonction", n->nom);
                erreur_execution(n->ligne, msg);
            }
            return appeler_fonction(cible.comme.fonction, args, nb, n->ligne);
        }

        case N_ACCES_MEMBRE: {
            Valeur cible = evaluer(n->cible, env);
            if (cible.type != V_MODULE) erreur_execution(n->ligne, "impossible d'acceder a un membre: ce n'est pas un module");
            Valeur v;
            if (!environnement_obtenir(cible.comme.module->environnement, n->nom, &v)) {
                char msg[220];
                snprintf(msg, sizeof(msg), "le module '%s' n'exporte pas '%s'", cible.comme.module->nom, n->nom);
                erreur_execution(n->ligne, msg);
            }
            return v;
        }

        case N_APPEL_METHODE: {
            Valeur cible = evaluer(n->cible, env);
            if (cible.type != V_MODULE) erreur_execution(n->ligne, "impossible d'appeler un membre: ce n'est pas un module");
            Valeur fonction_v;
            if (!environnement_obtenir(cible.comme.module->environnement, n->nom, &fonction_v)) {
                char msg[220];
                snprintf(msg, sizeof(msg), "le module '%s' n'exporte pas '%s'", cible.comme.module->nom, n->nom);
                erreur_execution(n->ligne, msg);
            }
            if (fonction_v.type != V_FONCTION) {
                char msg[220];
                snprintf(msg, sizeof(msg), "'%s.%s' n'est pas une fonction", cible.comme.module->nom, n->nom);
                erreur_execution(n->ligne, msg);
            }
            Valeur args[64];
            int nb = n->nb_enfants;
            if (nb > 64) erreur_execution(n->ligne, "trop d'arguments");
            for (int i = 0; i < nb; i++) args[i] = evaluer(n->enfants[i], env);
            return appeler_fonction(fonction_v.comme.fonction, args, nb, n->ligne);
        }

        default:
            erreur_execution(n->ligne, "noeud d'expression inconnu");
            return valeur_rien();
    }
}

static void executer_importation(Noeud *n, Environnement *env) {
    char chemin_absolu[4096];
    joindre_chemin(chemin_absolu, sizeof(chemin_absolu), g_dossier_actuel, n->texte);

    char alias_calcule[256];
    const char *alias = n->nom;
    if (!alias) {
        const char *sep1 = strrchr(n->texte, '/');
        const char *sep2 = strrchr(n->texte, '\\');
        const char *base = (sep2 && (!sep1 || sep2 > sep1)) ? sep2 : sep1;
        base = base ? base + 1 : n->texte;
        snprintf(alias_calcule, sizeof(alias_calcule), "%s", base);
        char *point = strrchr(alias_calcule, '.');
        if (point) *point = '\0';
        alias = alias_calcule;
    }

    Valeur module_existant;
    if (chercher_module(chemin_absolu, &module_existant)) {
        environnement_definir(env, alias, module_existant);
        return;
    }

    if (module_en_cours_de_chargement(chemin_absolu)) {
        char msg[300];
        snprintf(msg, sizeof(msg), "import circulaire detecte: '%s'", n->texte);
        erreur_execution(n->ligne, msg);
    }

    FILE *f = fopen(chemin_absolu, "rb");
    if (!f) {
        char msg[350];
        snprintf(msg, sizeof(msg), "impossible d'importer '%s'", n->texte);
        erreur_execution(n->ligne, msg);
    }
    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *source = malloc(taille + 1);
    size_t lu = fread(source, 1, (size_t)taille, f);
    source[lu] = '\0';
    fclose(f);

    ListeJetons jetons = lexer_tokeniser(source);
    Noeud *programme_module = analyseur_analyser(jetons);
    Environnement *env_module = environnement_creer(NULL);

    char dossier_precedent[4096];
    snprintf(dossier_precedent, sizeof(dossier_precedent), "%s", g_dossier_actuel);
    obtenir_dossier(g_dossier_actuel, sizeof(g_dossier_actuel), chemin_absolu);

    if (g_nb_modules_en_cours < MAX_IMPORTS_EN_COURS) {
        g_modules_en_cours[g_nb_modules_en_cours++] = strdup(chemin_absolu);
    }

    executer_bloc(programme_module->instructions, programme_module->nb_instructions, env_module);

    if (g_nb_modules_en_cours > 0) g_nb_modules_en_cours--;
    snprintf(g_dossier_actuel, sizeof(g_dossier_actuel), "%s", dossier_precedent);

    ModuleVal *m = malloc(sizeof(ModuleVal));
    m->nom = strdup(alias);
    m->environnement = env_module;
    Valeur v = valeur_module(m);

    enregistrer_module(chemin_absolu, v);
    environnement_definir(env, alias, v);
}

/* ------------------------------------------------------------------ */
/* execution d'instructions                                           */

static HookInstruction hook_avant_instruction = NULL;

void interpreteur_definir_hook(HookInstruction hook) {
    hook_avant_instruction = hook;
}

static Resultat executer_instruction(Noeud *n, Environnement *env) {
    if (hook_avant_instruction) hook_avant_instruction(n, env);
    switch (n->type) {
        case N_DECLARATION:
            environnement_definir(env, n->nom, evaluer(n->expression, env));
            return resultat_normal();

        case N_AFFECTATION:
            if (!environnement_assigner(env, n->nom, evaluer(n->expression, env))) {
                char msg[160];
                snprintf(msg, sizeof(msg), "impossible d'assigner, variable non declaree: '%s'", n->nom);
                erreur_execution(n->ligne, msg);
            }
            return resultat_normal();

        case N_AFFECTATION_INDEX: {
            Valeur conteneur = evaluer(n->cible, env);
            Valeur index = evaluer(n->index, env);
            Valeur valeur = evaluer(n->expression, env);
            if (conteneur.type != V_LISTE) erreur_execution(n->ligne, "impossible d'indexer cette valeur");
            if (!valeur_est_nombre(index)) erreur_execution(n->ligne, "index invalide");
            int idx = indice_reel(valeur_comme_reel(index), conteneur.comme.liste->compte, n->ligne);
            conteneur.comme.liste->elements[idx] = valeur;
            return resultat_normal();
        }

        case N_AFFICHE: {
            for (int i = 0; i < n->nb_enfants; i++) {
                if (i > 0) printf(" ");
                char *s = valeur_formater(evaluer(n->enfants[i], env));
                printf("%s", s);
                free(s);
            }
            printf("\n");
            return resultat_normal();
        }

        case N_DEMANDE: {
            if (n->invite) { printf("%s", n->invite); fflush(stdout); }
            char ligne_lue[4096];
            if (!fgets(ligne_lue, sizeof(ligne_lue), stdin)) ligne_lue[0] = '\0';
            size_t len = strlen(ligne_lue);
            while (len > 0 && (ligne_lue[len - 1] == '\n' || ligne_lue[len - 1] == '\r')) ligne_lue[--len] = '\0';

            char *fin_parse;
            long long entier_lu = strtoll(ligne_lue, &fin_parse, 10);
            if (len > 0 && *fin_parse == '\0') {
                environnement_definir(env, n->nom, valeur_entier(entier_lu));
            } else {
                double reel_lu = strtod(ligne_lue, &fin_parse);
                if (len > 0 && *fin_parse == '\0') {
                    environnement_definir(env, n->nom, valeur_reel(reel_lu));
                } else {
                    environnement_definir(env, n->nom, valeur_texte(ligne_lue));
                }
            }
            return resultat_normal();
        }

        case N_SI: {
            for (int i = 0; i < n->nb_branches; i++) {
                if (valeur_est_vraie(evaluer(n->branches[i].condition, env))) {
                    Environnement *sous_env = environnement_creer(env);
                    Resultat r = executer_bloc(n->branches[i].bloc, n->branches[i].nb_bloc, sous_env);
                    environnement_detruire(sous_env);
                    return r;
                }
            }
            if (n->sinon) {
                Environnement *sous_env = environnement_creer(env);
                Resultat r = executer_bloc(n->sinon, n->nb_sinon, sous_env);
                environnement_detruire(sous_env);
                return r;
            }
            return resultat_normal();
        }

        case N_TANTQUE: {
            while (valeur_est_vraie(evaluer(n->expression, env))) {
                Environnement *sous_env = environnement_creer(env);
                Resultat r = executer_bloc(n->bloc, n->nb_bloc, sous_env);
                environnement_detruire(sous_env);
                if (r.statut == STATUT_ARRETE) break;
                if (r.statut == STATUT_RETOURNE) return r;
            }
            return resultat_normal();
        }

        case N_POUR: {
            Valeur depart = evaluer(n->depart, env);
            Valeur fin = evaluer(n->fin, env);
            Valeur pas = n->pas ? evaluer(n->pas, env) : valeur_entier(1);
            double p = valeur_comme_reel(pas);
            if (p == 0.0) erreur_execution(n->ligne, "le pas d'une boucle 'pour' ne peut pas etre zero");

            int tout_entier = depart.type == V_ENTIER && fin.type == V_ENTIER && pas.type == V_ENTIER;
            double valeur_d = valeur_comme_reel(depart);
            double fin_d = valeur_comme_reel(fin);

            while ((p > 0 && valeur_d <= fin_d) || (p < 0 && valeur_d >= fin_d)) {
                Environnement *sous_env = environnement_creer(env);
                Valeur v_boucle = tout_entier ? valeur_entier((int64_t)valeur_d) : valeur_reel(valeur_d);
                environnement_definir(sous_env, n->nom, v_boucle);
                Resultat r = executer_bloc(n->bloc, n->nb_bloc, sous_env);
                environnement_detruire(sous_env);
                if (r.statut == STATUT_ARRETE) break;
                if (r.statut == STATUT_RETOURNE) return r;
                valeur_d += p;
            }
            return resultat_normal();
        }

        case N_POUR_CHAQUE: {
            Valeur iterable = evaluer(n->expression, env);
            if (iterable.type == V_LISTE) {
                for (int i = 0; i < iterable.comme.liste->compte; i++) {
                    Environnement *sous_env = environnement_creer(env);
                    environnement_definir(sous_env, n->nom, iterable.comme.liste->elements[i]);
                    Resultat r = executer_bloc(n->bloc, n->nb_bloc, sous_env);
                    environnement_detruire(sous_env);
                    if (r.statut == STATUT_ARRETE) break;
                    if (r.statut == STATUT_RETOURNE) return r;
                }
            } else if (iterable.type == V_TEXTE) {
                size_t taille = strlen(iterable.comme.texte);
                for (size_t i = 0; i < taille; i++) {
                    Environnement *sous_env = environnement_creer(env);
                    char car[2] = { iterable.comme.texte[i], '\0' };
                    environnement_definir(sous_env, n->nom, valeur_texte(car));
                    Resultat r = executer_bloc(n->bloc, n->nb_bloc, sous_env);
                    environnement_detruire(sous_env);
                    if (r.statut == STATUT_ARRETE) break;
                    if (r.statut == STATUT_RETOURNE) return r;
                }
            } else {
                erreur_execution(n->ligne, "'pour chaque' attend une liste ou un texte");
            }
            return resultat_normal();
        }

        case N_FONCTION_DEF: {
            FonctionVal *f = malloc(sizeof(FonctionVal));
            f->nom = n->nom;
            f->parametres = n->parametres;
            f->nb_parametres = n->nb_parametres;
            f->bloc = n->bloc;
            f->nb_instructions = n->nb_bloc;
            f->env_definition = env;
            environnement_definir(env, n->nom, valeur_fonction(f));
            return resultat_normal();
        }

        case N_RETOURNE: {
            Resultat r;
            r.statut = STATUT_RETOURNE;
            r.valeur = n->expression ? evaluer(n->expression, env) : valeur_rien();
            return r;
        }

        case N_ARRETE: { Resultat r; r.statut = STATUT_ARRETE; r.valeur = valeur_rien(); return r; }
        case N_CONTINUE: { Resultat r; r.statut = STATUT_CONTINUE; r.valeur = valeur_rien(); return r; }

        case N_EXPR_INSTRUCTION:
            evaluer(n->expression, env);
            return resultat_normal();

        case N_IMPORTATION:
            executer_importation(n, env);
            return resultat_normal();

        default:
            erreur_execution(n->ligne, "instruction inconnue");
            return resultat_normal();
    }
}

static Resultat executer_bloc(Noeud **instructions, int n, Environnement *env) {
    for (int i = 0; i < n; i++) {
        Resultat r = executer_instruction(instructions[i], env);
        if (r.statut != STATUT_NORMAL) return r;
    }
    return resultat_normal();
}

void interpreteur_executer(Noeud *programme, Environnement *globales) {
    executer_bloc(programme->instructions, programme->nb_instructions, globales);
}

void interpreteur_erreur(int ligne, const char *msg) {
    erreur_execution(ligne, msg);
}
