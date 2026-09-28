#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aide.h"

static ResultatExecution executer_debogueur(const char *source, const char *lignes_arret, const char *commandes) {
    char chemin_script[] = "/tmp/test_easylang_dbg_XXXXXX.elg";
    int fd = mkstemps(chemin_script, 4);
    write(fd, source, strlen(source));
    close(fd);

    char chemin_commandes[] = "/tmp/test_easylang_cmd_XXXXXX";
    int fd_cmd = mkstemp(chemin_commandes);
    write(fd_cmd, commandes, strlen(commandes));
    close(fd_cmd);

    char commande[512];
    snprintf(commande, sizeof(commande), "../easy_debogueur '%s' %s < '%s' 2>&1",
             chemin_script, lignes_arret ? lignes_arret : "", chemin_commandes);

    FILE *p = popen(commande, "r");
    size_t capacite = 4096, taille = 0;
    char *tampon = malloc(capacite);
    tampon[0] = '\0';
    char morceau[1024];
    size_t lu;
    while ((lu = fread(morceau, 1, sizeof(morceau), p)) > 0) {
        if (taille + lu + 1 > capacite) { capacite = (taille + lu + 1) * 2; tampon = realloc(tampon, capacite); }
        memcpy(tampon + taille, morceau, lu);
        taille += lu;
        tampon[taille] = '\0';
    }
    int statut = pclose(p);

    unlink(chemin_script);
    unlink(chemin_commandes);

    ResultatExecution r;
    r.sortie = tampon;
    r.code_sortie = WIFEXITED(statut) ? WEXITSTATUS(statut) : -1;
    return r;
}

static void test_pas_a_pas_sans_point_arret(void) {
    ResultatExecution r = executer_debogueur("soit x = 1\nsoit y = 2\naffiche x + y\n", NULL, "n\nn\nn\n");
    VERIFIER_CONTIENT(r.sortie, "ligne 1");
    VERIFIER_CONTIENT(r.sortie, "ligne 2");
    VERIFIER_CONTIENT(r.sortie, "ligne 3");
    VERIFIER_CONTIENT(r.sortie, "3\n");
    executer_script_liberer(r);
}

static void test_point_arret_ignore_lignes_precedentes(void) {
    ResultatExecution r = executer_debogueur("soit x = 1\nsoit y = 2\naffiche x + y\n", "3", "c\n");
    VERIFIER(strstr(r.sortie, "ligne 1") == NULL);
    VERIFIER(strstr(r.sortie, "ligne 2") == NULL);
    VERIFIER_CONTIENT(r.sortie, "ligne 3");
    executer_script_liberer(r);
}

static void test_inspection_variable(void) {
    ResultatExecution r = executer_debogueur("soit x = 42\naffiche x\n", "2", "p x\nc\n");
    VERIFIER_CONTIENT(r.sortie, "x = 42");
    executer_script_liberer(r);
}

static void test_liste_variables(void) {
    ResultatExecution r = executer_debogueur("soit a = 1\nsoit b = 2\naffiche a\n", "3", "v\nc\n");
    VERIFIER_CONTIENT(r.sortie, "a = 1");
    VERIFIER_CONTIENT(r.sortie, "b = 2");
    executer_script_liberer(r);
}

static void test_quitter(void) {
    ResultatExecution r = executer_debogueur("soit x = 1\naffiche x\n", NULL, "q\n");
    VERIFIER_CONTIENT(r.sortie, "arretee par l'utilisateur");
    VERIFIER(strstr(r.sortie, "ligne 2") == NULL); /* affiche x (ligne 2) n'a jamais tourne */
    executer_script_liberer(r);
}

static void test_ajout_point_arret_dynamique(void) {
    ResultatExecution r = executer_debogueur("soit x = 1\nsoit y = 2\naffiche y\n", NULL, "ba 3\nc\nc\n");
    VERIFIER_CONTIENT(r.sortie, "ligne 1");
    VERIFIER(strstr(r.sortie, "ligne 2") == NULL);
    VERIFIER_CONTIENT(r.sortie, "ligne 3");
    executer_script_liberer(r);
}

void executer_tests_debogueur(void) {
    test_pas_a_pas_sans_point_arret();
    test_point_arret_ignore_lignes_precedentes();
    test_inspection_variable();
    test_liste_variables();
    test_quitter();
    test_ajout_point_arret_dynamique();
}
