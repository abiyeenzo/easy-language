#include "aide.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int TESTS_TOTAL = 0;
int TESTS_ECHOUES = 0;

void verifier_impl(int condition, const char *message, const char *fichier, int ligne) {
    TESTS_TOTAL++;
    if (!condition) {
        TESTS_ECHOUES++;
        fprintf(stderr, "ECHEC %s:%d: %s\n", fichier, ligne, message);
    }
}

void verifier_egal_str_impl(const char *obtenu, const char *attendu, const char *fichier, int ligne) {
    TESTS_TOTAL++;
    if (strcmp(obtenu, attendu) != 0) {
        TESTS_ECHOUES++;
        fprintf(stderr, "ECHEC %s:%d:\n  attendu: %s\n  obtenu:  %s\n", fichier, ligne, attendu, obtenu);
    }
}

void verifier_contient_impl(const char *obtenu, const char *sous_chaine, const char *fichier, int ligne) {
    TESTS_TOTAL++;
    if (strstr(obtenu, sous_chaine) == NULL) {
        TESTS_ECHOUES++;
        fprintf(stderr, "ECHEC %s:%d:\n  attendu (sous-chaine): %s\n  obtenu:  %s\n", fichier, ligne, sous_chaine, obtenu);
    }
}

static ResultatExecution executer_script_impl(const char *source, const char *entree, const char *dossier) {
    char chemin_script[512];
    snprintf(chemin_script, sizeof(chemin_script), "%s/test_easylang_XXXXXX.elg", dossier);
    int fd = mkstemps(chemin_script, 4);
    if (fd < 0) { perror("mkstemps"); exit(1); }
    write(fd, source, strlen(source));
    close(fd);

    char chemin_entree[] = "/tmp/test_easylang_in_XXXXXX";
    if (entree) {
        int fd_entree = mkstemp(chemin_entree);
        write(fd_entree, entree, strlen(entree));
        close(fd_entree);
    }

    char commande[1200];
    if (entree) {
        snprintf(commande, sizeof(commande), "../easylang '%s' < '%s' 2>&1", chemin_script, chemin_entree);
    } else {
        snprintf(commande, sizeof(commande), "../easylang '%s' < /dev/null 2>&1", chemin_script);
    }

    FILE *p = popen(commande, "r");
    size_t capacite = 4096, taille = 0;
    char *tampon = malloc(capacite);
    tampon[0] = '\0';
    char morceau[1024];
    size_t lu;
    while ((lu = fread(morceau, 1, sizeof(morceau), p)) > 0) {
        if (taille + lu + 1 > capacite) {
            capacite = (taille + lu + 1) * 2;
            tampon = realloc(tampon, capacite);
        }
        memcpy(tampon + taille, morceau, lu);
        taille += lu;
        tampon[taille] = '\0';
    }
    int statut = pclose(p);

    unlink(chemin_script);
    if (entree) unlink(chemin_entree);

    ResultatExecution r;
    r.sortie = tampon;
    r.code_sortie = WIFEXITED(statut) ? WEXITSTATUS(statut) : -1;
    return r;
}

ResultatExecution executer_script(const char *source, const char *entree) {
    return executer_script_impl(source, entree, "/tmp");
}

ResultatExecution executer_script_dans_dossier(const char *source, const char *entree, const char *dossier) {
    return executer_script_impl(source, entree, dossier);
}

void executer_script_liberer(ResultatExecution r) {
    free(r.sortie);
}
