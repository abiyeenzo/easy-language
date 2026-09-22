#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *lire_fichier(const char *chemin) {
    FILE *f = fopen(chemin, "rb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible de lire le fichier '%s'\n", chemin);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *contenu = malloc(taille + 1);
    size_t lu = fread(contenu, 1, taille, f);
    contenu[lu] = '\0';
    fclose(f);
    return contenu;
}

char **decouper_lignes(const char *source, int *nb_dehors) {
    int capacite = 64;
    char **lignes = malloc(sizeof(char *) * capacite);
    int compte = 0;

    const char *debut = source;
    const char *p = source;
    for (;;) {
        if (*p == '\n' || *p == '\0') {
            int longueur = (int)(p - debut);
            if (longueur > 0 && debut[longueur - 1] == '\r') longueur--;
            char *l = malloc(longueur + 1);
            memcpy(l, debut, longueur);
            l[longueur] = '\0';
            if (compte >= capacite) { capacite *= 2; lignes = realloc(lignes, sizeof(char *) * capacite); }
            lignes[compte++] = l;
            if (*p == '\0') break;
            debut = p + 1;
        }
        p++;
    }
    *nb_dehors = compte;
    return lignes;
}
