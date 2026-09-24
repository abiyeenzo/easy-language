#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

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

void obtenir_dossier(char *dehors, size_t taille, const char *chemin) {
    const char *sep1 = strrchr(chemin, '/');
    const char *sep2 = strrchr(chemin, '\\');
    const char *sep = (sep2 && (!sep1 || sep2 > sep1)) ? sep2 : sep1;
    if (!sep) { snprintf(dehors, taille, "."); return; }
    size_t longueur = (size_t)(sep - chemin);
    if (longueur >= taille) longueur = taille - 1;
    memcpy(dehors, chemin, longueur);
    dehors[longueur] = '\0';
}

void obtenir_dossier_executable(char *dehors, size_t taille) {
    char chemin_exe[4096];
#ifdef _WIN32
    DWORD lu = GetModuleFileNameA(NULL, chemin_exe, sizeof(chemin_exe));
    if (lu == 0 || lu >= sizeof(chemin_exe)) { snprintf(dehors, taille, "."); return; }
#else
    ssize_t lu = readlink("/proc/self/exe", chemin_exe, sizeof(chemin_exe) - 1);
    if (lu <= 0) { snprintf(dehors, taille, "."); return; }
    chemin_exe[lu] = '\0';
#endif
    obtenir_dossier(dehors, taille, chemin_exe);
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
