#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

char *lire_fichier(const char *chemin);

/* Ecrit dans 'dehors' le dossier contenant 'chemin' (sans le separateur
   final), ou "." si 'chemin' ne contient aucun separateur. */
void obtenir_dossier(char *dehors, size_t taille, const char *chemin);

/* Ecrit dans 'dehors' le dossier contenant l'executable en cours (sans le
   separateur final), ou "." si indeterminable. Sert a localiser la
   bibliotheque standard fournie a cote de l'executable (dossier
   bibliotheque/), independamment du dossier du script en cours
   d'execution : voir "importe nom" (sans chemin) dans interpreteur.c. */
void obtenir_dossier_executable(char *dehors, size_t taille);

/* Decoupe une source en lignes (sans '\n' ni '\r' final). *nb_dehors recoit
   le nombre de lignes. Le tableau et ses chaines sont alloues (jamais
   liberes, comme le reste de ce projet). */
char **decouper_lignes(const char *source, int *nb_dehors);

#endif
