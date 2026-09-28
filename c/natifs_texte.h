#ifndef NATIFS_TEXTE_H
#define NATIFS_TEXTE_H

#include "valeur.h"

int natifs_texte_est(const char *nom);
Valeur natifs_texte_appeler(const char *nom, Valeur *args, int nb_args, int ligne);

#endif
