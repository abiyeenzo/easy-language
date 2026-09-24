#ifndef NATIFS_RESEAU_H
#define NATIFS_RESEAU_H

#include "valeur.h"

int natifs_reseau_est(const char *nom);
Valeur natifs_reseau_appeler(const char *nom, Valeur *args, int nb_args, int ligne);

#endif
