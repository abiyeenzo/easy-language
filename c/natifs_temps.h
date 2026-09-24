#ifndef NATIFS_TEMPS_H
#define NATIFS_TEMPS_H

#include "valeur.h"

int natifs_temps_est(const char *nom);
Valeur natifs_temps_appeler(const char *nom, Valeur *args, int nb_args, int ligne);

#endif
