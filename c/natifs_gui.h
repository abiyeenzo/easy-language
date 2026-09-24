#ifndef NATIFS_GUI_H
#define NATIFS_GUI_H

#include "valeur.h"

int natifs_gui_est(const char *nom);
Valeur natifs_gui_appeler(const char *nom, Valeur *args, int nb_args, int ligne);

#endif
