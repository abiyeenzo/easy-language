#include "valeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Valeur valeur_rien(void) {
    Valeur v;
    v.type = V_RIEN;
    return v;
}

Valeur valeur_entier(int64_t n) {
    Valeur v;
    v.type = V_ENTIER;
    v.comme.entier = n;
    return v;
}

Valeur valeur_reel(double n) {
    Valeur v;
    v.type = V_REEL;
    v.comme.reel = n;
    return v;
}

Valeur valeur_booleen(int b) {
    Valeur v;
    v.type = V_BOOLEEN;
    v.comme.booleen = b ? 1 : 0;
    return v;
}

Valeur valeur_texte(const char *s) {
    Valeur v;
    v.type = V_TEXTE;
    v.comme.texte = strdup(s);
    return v;
}

Valeur valeur_liste_vide(void) {
    Valeur v;
    v.type = V_LISTE;
    Liste *l = malloc(sizeof(Liste));
    l->compte = 0;
    l->capacite = 4;
    l->elements = malloc(sizeof(Valeur) * l->capacite);
    v.comme.liste = l;
    return v;
}

Valeur valeur_fonction(FonctionVal *f) {
    Valeur v;
    v.type = V_FONCTION;
    v.comme.fonction = f;
    return v;
}

Valeur valeur_module(ModuleVal *m) {
    Valeur v;
    v.type = V_MODULE;
    v.comme.module = m;
    return v;
}

int valeur_est_vraie(Valeur v) {
    switch (v.type) {
        case V_RIEN: return 0;
        case V_BOOLEEN: return v.comme.booleen;
        case V_ENTIER: return v.comme.entier != 0;
        case V_REEL: return v.comme.reel != 0.0;
        case V_TEXTE: return v.comme.texte[0] != '\0';
        case V_LISTE: return v.comme.liste->compte > 0;
        case V_FONCTION: return 1;
        case V_MODULE: return 1;
    }
    return 0;
}

int valeur_est_nombre(Valeur v) {
    return v.type == V_ENTIER || v.type == V_REEL;
}

double valeur_comme_reel(Valeur v) {
    return v.type == V_ENTIER ? (double)v.comme.entier : v.comme.reel;
}

int valeur_egales(Valeur a, Valeur b) {
    if (a.type != b.type) {
        if (valeur_est_nombre(a) && valeur_est_nombre(b)) {
            return valeur_comme_reel(a) == valeur_comme_reel(b);
        }
        return 0;
    }
    switch (a.type) {
        case V_RIEN: return 1;
        case V_BOOLEEN: return a.comme.booleen == b.comme.booleen;
        case V_ENTIER: return a.comme.entier == b.comme.entier;
        case V_REEL: return a.comme.reel == b.comme.reel;
        case V_TEXTE: return strcmp(a.comme.texte, b.comme.texte) == 0;
        case V_LISTE: {
            Liste *la = a.comme.liste, *lb = b.comme.liste;
            if (la->compte != lb->compte) return 0;
            for (int i = 0; i < la->compte; i++) {
                if (!valeur_egales(la->elements[i], lb->elements[i])) return 0;
            }
            return 1;
        }
        case V_FONCTION: return a.comme.fonction == b.comme.fonction;
        case V_MODULE: return a.comme.module == b.comme.module;
    }
    return 0;
}

char *valeur_formater(Valeur v) {
    char tampon[64];
    switch (v.type) {
        case V_RIEN:
            return strdup("rien");
        case V_BOOLEEN:
            return strdup(v.comme.booleen ? "vrai" : "faux");
        case V_ENTIER:
            snprintf(tampon, sizeof(tampon), "%lld", (long long)v.comme.entier);
            return strdup(tampon);
        case V_REEL: {
            snprintf(tampon, sizeof(tampon), "%g", v.comme.reel);
            return strdup(tampon);
        }
        case V_TEXTE:
            return strdup(v.comme.texte);
        case V_FONCTION:
            snprintf(tampon, sizeof(tampon), "<fonction %s>", v.comme.fonction->nom);
            return strdup(tampon);
        case V_MODULE:
            snprintf(tampon, sizeof(tampon), "<module %s>", v.comme.module->nom);
            return strdup(tampon);
        case V_LISTE: {
            Liste *l = v.comme.liste;
            size_t taille = 3;
            char **parties = malloc(sizeof(char *) * (l->compte > 0 ? l->compte : 1));
            for (int i = 0; i < l->compte; i++) {
                parties[i] = valeur_formater(l->elements[i]);
                taille += strlen(parties[i]) + 1;
            }
            char *resultat = malloc(taille);
            resultat[0] = '\0';
            strcat(resultat, "[");
            for (int i = 0; i < l->compte; i++) {
                if (i > 0) strcat(resultat, " ");
                strcat(resultat, parties[i]);
                free(parties[i]);
            }
            strcat(resultat, "]");
            free(parties);
            return resultat;
        }
    }
    return strdup("");
}

void liste_ajoute(Liste *liste, Valeur v) {
    if (liste->compte >= liste->capacite) {
        liste->capacite *= 2;
        liste->elements = realloc(liste->elements, sizeof(Valeur) * liste->capacite);
    }
    liste->elements[liste->compte++] = v;
}
