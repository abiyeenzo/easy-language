#include "natifs_os.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

#include "interpreteur.h"

int natifs_os_est(const char *nom) {
    static const char *noms[] = {
        "fichier_existe", "lire_fichier", "ecrire_fichier", "ajouter_fichier",
        "supprimer_fichier", "repertoire_courant", "variable_environnement",
        "horodatage", "dormir",
    };
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

static void exiger_texte(Valeur v, int ligne, const char *nom_fonction) {
    if (v.type != V_TEXTE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' attend du texte", nom_fonction);
        interpreteur_erreur(ligne, msg);
    }
}

Valeur natifs_os_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "fichier_existe") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'fichier_existe' attend 1 argument");
        exiger_texte(args[0], ligne, "fichier_existe");
        FILE *f = fopen(args[0].comme.texte, "rb");
        if (!f) return valeur_booleen(0);
        fclose(f);
        return valeur_booleen(1);
    }

    if (strcmp(nom, "lire_fichier") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'lire_fichier' attend 1 argument");
        exiger_texte(args[0], ligne, "lire_fichier");
        FILE *f = fopen(args[0].comme.texte, "rb");
        if (!f) return valeur_rien(); /* fichier absent: cas normal, pas une erreur de programme */
        fseek(f, 0, SEEK_END);
        long taille = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *tampon = malloc((size_t)taille + 1);
        size_t lu = fread(tampon, 1, (size_t)taille, f);
        tampon[lu] = '\0';
        fclose(f);
        Valeur v = valeur_texte(tampon);
        free(tampon);
        return v;
    }

    if (strcmp(nom, "ecrire_fichier") == 0 || strcmp(nom, "ajouter_fichier") == 0) {
        int ajout = strcmp(nom, "ajouter_fichier") == 0;
        if (nb_args != 2) interpreteur_erreur(ligne, "attend 2 arguments: chemin et contenu");
        exiger_texte(args[0], ligne, nom);
        char *contenu = valeur_formater(args[1]);
        FILE *f = fopen(args[0].comme.texte, ajout ? "ab" : "wb");
        if (!f) { free(contenu); return valeur_booleen(0); }
        size_t longueur = strlen(contenu);
        size_t ecrit = fwrite(contenu, 1, longueur, f);
        fclose(f);
        free(contenu);
        return valeur_booleen(ecrit == longueur);
    }

    if (strcmp(nom, "supprimer_fichier") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'supprimer_fichier' attend 1 argument");
        exiger_texte(args[0], ligne, "supprimer_fichier");
        return valeur_booleen(remove(args[0].comme.texte) == 0);
    }

    if (strcmp(nom, "repertoire_courant") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'repertoire_courant' n'attend aucun argument");
        char tampon[4096];
        if (!getcwd(tampon, sizeof(tampon))) return valeur_texte(".");
        return valeur_texte(tampon);
    }

    if (strcmp(nom, "variable_environnement") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'variable_environnement' attend 1 argument");
        exiger_texte(args[0], ligne, "variable_environnement");
        const char *v = getenv(args[0].comme.texte);
        if (!v) return valeur_rien();
        return valeur_texte(v);
    }

    if (strcmp(nom, "horodatage") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'horodatage' n'attend aucun argument");
        return valeur_entier((int64_t)time(NULL));
    }

    if (strcmp(nom, "dormir") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'dormir' attend 1 argument");
        if (!valeur_est_nombre(args[0])) interpreteur_erreur(ligne, "'dormir' attend un nombre de secondes");
        double secondes = valeur_comme_reel(args[0]);
        if (secondes < 0) secondes = 0;
#ifdef _WIN32
        Sleep((DWORD)(secondes * 1000.0));
#else
        usleep((useconds_t)(secondes * 1000000.0));
#endif
        return valeur_rien();
    }

    return valeur_rien();
}
