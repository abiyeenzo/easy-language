#ifndef DEBOGUEUR_H
#define DEBOGUEUR_H

/* Installe le hook de debogage. 'lignes'/'nb_lignes' sont le source
   decoupe (pour l'affichage de contexte). 'points'/'nb_points' sont les
   numeros de ligne des points d'arret initiaux ; si nb_points == 0, le
   debogueur demarre en mode pas-a-pas (s'arrete a la toute premiere
   instruction). */
void debogueur_installer(char **lignes, int nb_lignes, int *points, int nb_points);

#endif
