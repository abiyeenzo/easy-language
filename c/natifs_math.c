#include "natifs_math.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "interpreteur.h"

static int graine_initialisee = 0;

static void assurer_graine(void) {
    if (!graine_initialisee) {
        srand((unsigned int)time(NULL));
        graine_initialisee = 1;
    }
}

static void exiger_nombre(Valeur v, int ligne, const char *nom_fonction) {
    if (!valeur_est_nombre(v)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' attend un nombre", nom_fonction);
        interpreteur_erreur(ligne, msg);
    }
}

int natifs_math_est(const char *nom) {
    static const char *noms[] = {
        "racine", "puissance", "sin", "cos", "tan", "abs",
        "plancher", "plafond", "arrondi", "log", "exp",
        "pi", "e", "alea", "alea_entier", "min", "max",
    };
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

Valeur natifs_math_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "racine") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'racine' attend 1 argument");
        exiger_nombre(args[0], ligne, "racine");
        double x = valeur_comme_reel(args[0]);
        if (x < 0) interpreteur_erreur(ligne, "racine() d'un nombre negatif");
        return valeur_reel(sqrt(x));
    }
    if (strcmp(nom, "puissance") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'puissance' attend 2 arguments");
        exiger_nombre(args[0], ligne, "puissance");
        exiger_nombre(args[1], ligne, "puissance");
        double resultat = pow(valeur_comme_reel(args[0]), valeur_comme_reel(args[1]));
        if (args[0].type == V_ENTIER && args[1].type == V_ENTIER && args[1].comme.entier >= 0) {
            return valeur_entier((int64_t)llround(resultat));
        }
        return valeur_reel(resultat);
    }
    if (strcmp(nom, "sin") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'sin' attend 1 argument");
        exiger_nombre(args[0], ligne, "sin");
        return valeur_reel(sin(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "cos") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'cos' attend 1 argument");
        exiger_nombre(args[0], ligne, "cos");
        return valeur_reel(cos(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "tan") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'tan' attend 1 argument");
        exiger_nombre(args[0], ligne, "tan");
        return valeur_reel(tan(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "abs") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'abs' attend 1 argument");
        exiger_nombre(args[0], ligne, "abs");
        if (args[0].type == V_ENTIER) {
            int64_t v = args[0].comme.entier;
            return valeur_entier(v < 0 ? -v : v);
        }
        return valeur_reel(fabs(args[0].comme.reel));
    }
    if (strcmp(nom, "plancher") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'plancher' attend 1 argument");
        exiger_nombre(args[0], ligne, "plancher");
        return valeur_entier((int64_t)floor(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "plafond") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'plafond' attend 1 argument");
        exiger_nombre(args[0], ligne, "plafond");
        return valeur_entier((int64_t)ceil(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "arrondi") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'arrondi' attend 1 argument");
        exiger_nombre(args[0], ligne, "arrondi");
        return valeur_entier((int64_t)llround(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "log") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'log' attend 1 argument");
        exiger_nombre(args[0], ligne, "log");
        double x = valeur_comme_reel(args[0]);
        if (x <= 0) interpreteur_erreur(ligne, "log() d'un nombre non positif");
        return valeur_reel(log(x));
    }
    if (strcmp(nom, "exp") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'exp' attend 1 argument");
        exiger_nombre(args[0], ligne, "exp");
        return valeur_reel(exp(valeur_comme_reel(args[0])));
    }
    if (strcmp(nom, "pi") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'pi' n'attend aucun argument");
        return valeur_reel(3.14159265358979323846);
    }
    if (strcmp(nom, "e") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'e' n'attend aucun argument");
        return valeur_reel(2.71828182845904523536);
    }
    if (strcmp(nom, "alea") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'alea' n'attend aucun argument");
        assurer_graine();
        return valeur_reel((double)rand() / ((double)RAND_MAX + 1.0));
    }
    if (strcmp(nom, "alea_entier") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'alea_entier' attend 2 arguments");
        exiger_nombre(args[0], ligne, "alea_entier");
        exiger_nombre(args[1], ligne, "alea_entier");
        assurer_graine();
        int64_t a = (int64_t)valeur_comme_reel(args[0]);
        int64_t b = (int64_t)valeur_comme_reel(args[1]);
        if (b < a) interpreteur_erreur(ligne, "alea_entier(a b) attend a <= b");
        int64_t etendue = b - a + 1;
        return valeur_entier(a + (int64_t)(rand() % etendue));
    }
    if (strcmp(nom, "min") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'min' attend 2 arguments");
        exiger_nombre(args[0], ligne, "min");
        exiger_nombre(args[1], ligne, "min");
        return valeur_comme_reel(args[0]) <= valeur_comme_reel(args[1]) ? args[0] : args[1];
    }
    if (strcmp(nom, "max") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'max' attend 2 arguments");
        exiger_nombre(args[0], ligne, "max");
        exiger_nombre(args[1], ligne, "max");
        return valeur_comme_reel(args[0]) >= valeur_comme_reel(args[1]) ? args[0] : args[1];
    }
    return valeur_rien();
}
