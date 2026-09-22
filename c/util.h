#ifndef UTIL_H
#define UTIL_H

char *lire_fichier(const char *chemin);

/* Decoupe une source en lignes (sans '\n' ni '\r' final). *nb_dehors recoit
   le nombre de lignes. Le tableau et ses chaines sont alloues (jamais
   liberes, comme le reste de ce projet). */
char **decouper_lignes(const char *source, int *nb_dehors);

#endif
