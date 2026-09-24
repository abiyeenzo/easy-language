#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

char *lire_fichier(const char *chemin);

/* Ecrit dans 'dehors' le dossier contenant 'chemin' (sans le separateur
   final), ou "." si 'chemin' ne contient aucun separateur. */
void obtenir_dossier(char *dehors, size_t taille, const char *chemin);

/* Decoupe une source en lignes (sans '\n' ni '\r' final). *nb_dehors recoit
   le nombre de lignes. Le tableau et ses chaines sont alloues (jamais
   liberes, comme le reste de ce projet). */
char **decouper_lignes(const char *source, int *nb_dehors);

#endif
