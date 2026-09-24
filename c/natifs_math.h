#ifndef NATIFS_MATH_H
#define NATIFS_MATH_H

#include "valeur.h"

int natifs_math_est(const char *nom);
Valeur natifs_math_appeler(const char *nom, Valeur *args, int nb_args, int ligne);

#endif
