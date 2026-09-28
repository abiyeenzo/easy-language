#ifndef NATIFS_OS_H
#define NATIFS_OS_H

#include "valeur.h"

int natifs_os_est(const char *nom);
Valeur natifs_os_appeler(const char *nom, Valeur *args, int nb_args, int ligne);

#endif
