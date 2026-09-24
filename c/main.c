#include <stdio.h>
#include <string.h>

#include "analyseur.h"
#include "environnement.h"
#include "interpreteur.h"
#include "lexer.h"
#include "util.h"

#define VERSION "1.0.4"

int main(int argc, char **argv) {
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("Easy Language %s (C)\n", VERSION);
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fichier.elg>\n", argv[0]);
        fprintf(stderr, "       %s --version\n", argv[0]);
        return 1;
    }

    char *source = lire_fichier(argv[1]);
    ListeJetons jetons = lexer_tokeniser(source);
    Noeud *programme = analyseur_analyser(jetons);
    Environnement *globales = environnement_creer(NULL);

    char dossier[4096];
    obtenir_dossier(dossier, sizeof(dossier), argv[1]);
    interpreteur_definir_dossier(dossier);

    interpreteur_executer(programme, globales);
    return 0;
}
