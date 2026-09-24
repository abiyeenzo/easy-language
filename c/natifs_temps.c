#include "natifs_temps.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "interpreteur.h"

static void exiger_horodatage(Valeur v, int ligne, const char *nom_fonction) {
    if (!valeur_est_nombre(v)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' attend un horodatage (nombre)", nom_fonction);
        interpreteur_erreur(ligne, msg);
    }
}

static struct tm obtenir_tm(Valeur v) {
    time_t t = (time_t)valeur_comme_reel(v);
    struct tm resultat;
    struct tm *p = localtime(&t);
    if (p) resultat = *p;
    else memset(&resultat, 0, sizeof(resultat));
    return resultat;
}

int natifs_temps_est(const char *nom) {
    static const char *noms[] = {
        "maintenant", "annee", "mois", "jour", "heure", "minute",
        "seconde", "jour_semaine", "formater",
    };
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

Valeur natifs_temps_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "maintenant") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'maintenant' n'attend aucun argument");
        return valeur_entier((int64_t)time(NULL));
    }

    if (strcmp(nom, "annee") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'annee' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "annee");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_year + 1900);
    }

    if (strcmp(nom, "mois") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'mois' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "mois");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_mon + 1);
    }

    if (strcmp(nom, "jour") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'jour' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "jour");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_mday);
    }

    if (strcmp(nom, "heure") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'heure' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "heure");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_hour);
    }

    if (strcmp(nom, "minute") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'minute' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "minute");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_min);
    }

    if (strcmp(nom, "seconde") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'seconde' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "seconde");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_sec);
    }

    if (strcmp(nom, "jour_semaine") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'jour_semaine' attend 1 argument: horodatage");
        exiger_horodatage(args[0], ligne, "jour_semaine");
        struct tm t = obtenir_tm(args[0]);
        return valeur_entier(t.tm_wday);
    }

    if (strcmp(nom, "formater") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'formater' attend 2 arguments: horodatage et motif");
        exiger_horodatage(args[0], ligne, "formater");
        if (args[1].type != V_TEXTE) interpreteur_erreur(ligne, "'formater' attend un motif texte");
        struct tm t = obtenir_tm(args[0]);
        char tampon[256];
        size_t ecrit = strftime(tampon, sizeof(tampon), args[1].comme.texte, &t);
        if (ecrit == 0) tampon[0] = '\0';
        return valeur_texte(tampon);
    }

    return valeur_rien();
}
