#include <stdio.h>
#include <stdlib.h>

#include "analyseur.h"
#include "debogueur.h"
#include "environnement.h"
#include "interpreteur.h"
#include "lexer.h"
#include "util.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fichier.elg> [lignes_arret...]\n", argv[0]);
        return 1;
    }

    char *source = lire_fichier(argv[1]);

    int points[64];
    int nb_points = 0;
    for (int i = 2; i < argc && nb_points < 64; i++) points[nb_points++] = atoi(argv[i]);

    int nb_lignes;
    char **lignes = decouper_lignes(source, &nb_lignes);

    ListeJetons jetons = lexer_tokeniser(source);
    Noeud *programme = analyseur_analyser(jetons);
    Environnement *globales = environnement_creer(NULL);

    debogueur_installer(lignes, nb_lignes, points, nb_points);

    printf("=== Debogueur Easy Language === (tapez 'aide' pour les commandes)\n");
    interpreteur_executer(programme, globales);
    printf("=== programme termine ===\n");
    return 0;
}
