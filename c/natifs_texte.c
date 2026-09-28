#include "natifs_texte.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreteur.h"

static void exiger_texte(Valeur v, int ligne, const char *nom_fonction) {
    if (v.type != V_TEXTE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' attend du texte", nom_fonction);
        interpreteur_erreur(ligne, msg);
    }
}

int natifs_texte_est(const char *nom) {
    static const char *noms[] = {
        "decouper", "joindre", "remplacer", "majuscules", "minuscules",
        "rogner", "commence_par", "finit_par", "sous_texte", "inverse",
        "position", "contient_texte",
    };
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

Valeur natifs_texte_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "decouper") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'decouper' attend 2 arguments: texte et separateur");
        exiger_texte(args[0], ligne, "decouper");
        exiger_texte(args[1], ligne, "decouper");
        const char *source = args[0].comme.texte;
        const char *sep = args[1].comme.texte;
        size_t sep_len = strlen(sep);
        Valeur resultat = valeur_liste_vide();
        if (sep_len == 0) interpreteur_erreur(ligne, "'decouper' attend un separateur non vide");
        const char *debut = source;
        const char *trouve;
        while ((trouve = strstr(debut, sep)) != NULL) {
            char *morceau = malloc((size_t)(trouve - debut) + 1);
            memcpy(morceau, debut, (size_t)(trouve - debut));
            morceau[trouve - debut] = '\0';
            liste_ajoute(resultat.comme.liste, valeur_texte(morceau));
            free(morceau);
            debut = trouve + sep_len;
        }
        liste_ajoute(resultat.comme.liste, valeur_texte(debut));
        return resultat;
    }

    if (strcmp(nom, "joindre") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'joindre' attend 2 arguments: liste et separateur");
        if (args[0].type != V_LISTE) interpreteur_erreur(ligne, "'joindre' attend une liste en premier argument");
        exiger_texte(args[1], ligne, "joindre");
        Liste *l = args[0].comme.liste;
        const char *sep = args[1].comme.texte;
        size_t sep_len = strlen(sep);
        size_t taille = 1;
        char **parties = malloc(sizeof(char *) * (l->compte > 0 ? (size_t)l->compte : 1));
        for (int i = 0; i < l->compte; i++) {
            parties[i] = valeur_formater(l->elements[i]);
            taille += strlen(parties[i]) + sep_len;
        }
        char *resultat = malloc(taille);
        resultat[0] = '\0';
        for (int i = 0; i < l->compte; i++) {
            if (i > 0) strcat(resultat, sep);
            strcat(resultat, parties[i]);
            free(parties[i]);
        }
        free(parties);
        Valeur v = valeur_texte(resultat);
        free(resultat);
        return v;
    }

    if (strcmp(nom, "remplacer") == 0) {
        if (nb_args != 3) interpreteur_erreur(ligne, "'remplacer' attend 3 arguments: texte, ancien et nouveau");
        exiger_texte(args[0], ligne, "remplacer");
        exiger_texte(args[1], ligne, "remplacer");
        exiger_texte(args[2], ligne, "remplacer");
        const char *source = args[0].comme.texte;
        const char *ancien = args[1].comme.texte;
        const char *nouveau = args[2].comme.texte;
        size_t ancien_len = strlen(ancien);
        if (ancien_len == 0) return valeur_texte(source);

        size_t nouveau_len = strlen(nouveau);
        size_t occurrences = 0;
        const char *p = source;
        while ((p = strstr(p, ancien)) != NULL) {
            occurrences++;
            p += ancien_len;
        }
        size_t taille = strlen(source) + occurrences * (nouveau_len > ancien_len ? nouveau_len - ancien_len : 0) + 1;
        char *resultat = malloc(taille);
        char *ecriture = resultat;
        const char *debut = source;
        const char *trouve;
        while ((trouve = strstr(debut, ancien)) != NULL) {
            memcpy(ecriture, debut, (size_t)(trouve - debut));
            ecriture += trouve - debut;
            memcpy(ecriture, nouveau, nouveau_len);
            ecriture += nouveau_len;
            debut = trouve + ancien_len;
        }
        strcpy(ecriture, debut);
        Valeur v = valeur_texte(resultat);
        free(resultat);
        return v;
    }

    if (strcmp(nom, "majuscules") == 0 || strcmp(nom, "minuscules") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "attend 1 argument: texte");
        exiger_texte(args[0], ligne, nom);
        int vers_maj = strcmp(nom, "majuscules") == 0;
        char *copie = strdup(args[0].comme.texte);
        for (char *c = copie; *c; c++) {
            *c = (char)(vers_maj ? toupper((unsigned char)*c) : tolower((unsigned char)*c));
        }
        Valeur v = valeur_texte(copie);
        free(copie);
        return v;
    }

    if (strcmp(nom, "rogner") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'rogner' attend 1 argument: texte");
        exiger_texte(args[0], ligne, "rogner");
        const char *source = args[0].comme.texte;
        const char *debut = source;
        while (*debut && isspace((unsigned char)*debut)) debut++;
        const char *fin = source + strlen(source);
        while (fin > debut && isspace((unsigned char)*(fin - 1))) fin--;
        char *resultat = malloc((size_t)(fin - debut) + 1);
        memcpy(resultat, debut, (size_t)(fin - debut));
        resultat[fin - debut] = '\0';
        Valeur v = valeur_texte(resultat);
        free(resultat);
        return v;
    }

    if (strcmp(nom, "commence_par") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'commence_par' attend 2 arguments: texte et prefixe");
        exiger_texte(args[0], ligne, "commence_par");
        exiger_texte(args[1], ligne, "commence_par");
        size_t len_prefixe = strlen(args[1].comme.texte);
        if (len_prefixe > strlen(args[0].comme.texte)) return valeur_booleen(0);
        return valeur_booleen(strncmp(args[0].comme.texte, args[1].comme.texte, len_prefixe) == 0);
    }

    if (strcmp(nom, "finit_par") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'finit_par' attend 2 arguments: texte et suffixe");
        exiger_texte(args[0], ligne, "finit_par");
        exiger_texte(args[1], ligne, "finit_par");
        size_t len_source = strlen(args[0].comme.texte);
        size_t len_suffixe = strlen(args[1].comme.texte);
        if (len_suffixe > len_source) return valeur_booleen(0);
        return valeur_booleen(strcmp(args[0].comme.texte + (len_source - len_suffixe), args[1].comme.texte) == 0);
    }

    if (strcmp(nom, "sous_texte") == 0) {
        if (nb_args != 3) interpreteur_erreur(ligne, "'sous_texte' attend 3 arguments: texte, debut et fin");
        exiger_texte(args[0], ligne, "sous_texte");
        if (!valeur_est_nombre(args[1]) || !valeur_est_nombre(args[2])) {
            interpreteur_erreur(ligne, "'sous_texte' attend des indices numeriques");
        }
        const char *source = args[0].comme.texte;
        int64_t longueur = (int64_t)strlen(source);
        int64_t debut = (int64_t)valeur_comme_reel(args[1]);
        int64_t fin = (int64_t)valeur_comme_reel(args[2]);
        if (debut < 0) debut = 0;
        if (fin > longueur) fin = longueur;
        if (debut >= fin) return valeur_texte("");
        char *resultat = malloc((size_t)(fin - debut) + 1);
        memcpy(resultat, source + debut, (size_t)(fin - debut));
        resultat[fin - debut] = '\0';
        Valeur v = valeur_texte(resultat);
        free(resultat);
        return v;
    }

    if (strcmp(nom, "inverse") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'inverse' attend 1 argument: texte");
        exiger_texte(args[0], ligne, "inverse");
        size_t longueur = strlen(args[0].comme.texte);
        char *resultat = malloc(longueur + 1);
        for (size_t i = 0; i < longueur; i++) {
            resultat[i] = args[0].comme.texte[longueur - 1 - i];
        }
        resultat[longueur] = '\0';
        Valeur v = valeur_texte(resultat);
        free(resultat);
        return v;
    }

    if (strcmp(nom, "position") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'position' attend 2 arguments: texte et recherche");
        exiger_texte(args[0], ligne, "position");
        exiger_texte(args[1], ligne, "position");
        const char *trouve = strstr(args[0].comme.texte, args[1].comme.texte);
        if (!trouve) return valeur_entier(-1);
        return valeur_entier((int64_t)(trouve - args[0].comme.texte));
    }

    if (strcmp(nom, "contient_texte") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'contient_texte' attend 2 arguments: texte et recherche");
        exiger_texte(args[0], ligne, "contient_texte");
        exiger_texte(args[1], ligne, "contient_texte");
        return valeur_booleen(strstr(args[0].comme.texte, args[1].comme.texte) != NULL);
    }

    return valeur_rien();
}
