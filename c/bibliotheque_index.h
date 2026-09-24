#ifndef BIBLIOTHEQUE_INDEX_H
#define BIBLIOTHEQUE_INDEX_H

typedef struct {
    const char *nom;
    const char *description;
} EntreeBibliotheque;

/* Bibliotheque standard fournie avec l'interpreteur (dossier bibliotheque/
   a cote de l'executable, cf. util.h:obtenir_dossier_executable). Source
   unique utilisee a la fois pour resoudre "importe nom" (sans chemin,
   voir interpreteur.c) et pour lister les modules disponibles depuis la
   ligne de commande (easylang --aide modules, voir main.c). */
extern const EntreeBibliotheque BIBLIOTHEQUE_INDEX[];
extern const int BIBLIOTHEQUE_INDEX_COMPTE;

/* Retourne 1 si 'nom' designe un module de la bibliotheque standard. */
int bibliotheque_index_connu(const char *nom);

#endif
