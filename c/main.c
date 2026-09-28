#include <stdio.h>
#include <string.h>

#include "analyseur.h"
#include "bibliotheque_index.h"
#include "environnement.h"
#include "interpreteur.h"
#include "lexer.h"
#include "util.h"

#define VERSION "1.0.9"

static void afficher_aide(const char *programme) {
    printf("Easy Language %s : interpreteur en ligne de commande.\n\n", VERSION);
    printf("Usage :\n");
    printf("  %s <fichier.elg>        execute un script Easy Language\n", programme);
    printf("  %s --version, -v        affiche la version\n", programme);
    printf("  %s --aide, -h, --help   affiche cette aide\n", programme);
    printf("  %s --aide modules       liste les modules de la bibliotheque standard\n", programme);
    printf("\n");
    printf("Documentation complete : https://github.com/abiyeenzo/easy-language\n");
}

static void afficher_modules(void) {
    printf("Bibliotheque standard (importe <nom>, sans chemin) :\n\n");
    for (int i = 0; i < BIBLIOTHEQUE_INDEX_COMPTE; i++) {
        printf("  %-8s %s\n", BIBLIOTHEQUE_INDEX[i].nom, BIBLIOTHEQUE_INDEX[i].description);
    }
    printf("\nExemple :\n  importe math\n  affiche math.racine(16)\n");
    printf("\nPour importer votre propre fichier .elg (au lieu de la bibliotheque\n");
    printf("standard), utilisez un chemin entre guillemets : importe \"chemin.elg\"\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        afficher_aide(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("Easy Language %s (C)\n", VERSION);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "--aide") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        afficher_aide(argv[0]);
        return 0;
    }
    if ((argc == 3 && strcmp(argv[1], "--aide") == 0 && strcmp(argv[2], "modules") == 0) ||
        (argc == 2 && strcmp(argv[1], "--modules") == 0)) {
        afficher_modules();
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fichier.elg>\n", argv[0]);
        fprintf(stderr, "       %s --aide\n", argv[0]);
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
