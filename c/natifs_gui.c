#include "natifs_gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreteur.h"

#ifdef _WIN32
#include <windows.h>

static wchar_t *vers_large(const char *texte) {
    int longueur = MultiByteToWideChar(CP_UTF8, 0, texte, -1, NULL, 0);
    wchar_t *large = malloc(sizeof(wchar_t) * longueur);
    MultiByteToWideChar(CP_UTF8, 0, texte, -1, large, longueur);
    return large;
}
#endif

int natifs_gui_est(const char *nom) {
    static const char *noms[] = {"boite_message", "boite_question"};
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

Valeur natifs_gui_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "boite_message") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'boite_message' attend 2 arguments: titre et texte");
        exiger_texte(args[0], ligne, "boite_message");
        exiger_texte(args[1], ligne, "boite_message");
#ifdef _WIN32
        wchar_t *titre = vers_large(args[0].comme.texte);
        wchar_t *texte = vers_large(args[1].comme.texte);
        MessageBoxW(NULL, texte, titre, MB_OK | MB_ICONINFORMATION);
        free(titre);
        free(texte);
#else
        /* Pas de fenetrage sur cette plateforme (dev/tests Linux) : repli
           console, pour que les scripts utilisant boite_message restent
           executables et testables ici. Sous Windows (.exe), une vraie
           boite de dialogue s'affiche via MessageBoxW. */
        printf("[%s] %s\n", args[0].comme.texte, args[1].comme.texte);
#endif
        return valeur_rien();
    }

    if (strcmp(nom, "boite_question") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'boite_question' attend 2 arguments: titre et texte");
        exiger_texte(args[0], ligne, "boite_question");
        exiger_texte(args[1], ligne, "boite_question");
#ifdef _WIN32
        wchar_t *titre = vers_large(args[0].comme.texte);
        wchar_t *texte = vers_large(args[1].comme.texte);
        int reponse = MessageBoxW(NULL, texte, titre, MB_YESNO | MB_ICONQUESTION);
        free(titre);
        free(texte);
        return valeur_booleen(reponse == IDYES);
#else
        printf("[%s] %s (o/n) ", args[0].comme.texte, args[1].comme.texte);
        fflush(stdout);
        char ligne_lue[16];
        if (!fgets(ligne_lue, sizeof(ligne_lue), stdin)) return valeur_booleen(0);
        return valeur_booleen(ligne_lue[0] == 'o' || ligne_lue[0] == 'O');
#endif
    }

    return valeur_rien();
}
