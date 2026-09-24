#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "aide.h"

#define X(source) executer_script((source), NULL)

/* --- math --- */

static void test_math_racine_puissance(void) {
    ResultatExecution r;
    r = X("affiche racine(16)\n"); VERIFIER_EGAL_STR(r.sortie, "4\n"); executer_script_liberer(r);
    r = X("affiche racine(2)\n"); VERIFIER_CONTIENT(r.sortie, "1.414"); executer_script_liberer(r);
    r = X("affiche puissance(2 10)\n"); VERIFIER_EGAL_STR(r.sortie, "1024\n"); executer_script_liberer(r);
    r = X("affiche puissance(2.0 2)\n"); VERIFIER_EGAL_STR(r.sortie, "4\n"); executer_script_liberer(r);
}

static void test_math_racine_negative_erreur(void) {
    ResultatExecution r = X("affiche racine(-4)\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_math_trigo(void) {
    ResultatExecution r;
    r = X("affiche sin(0)\n"); VERIFIER_EGAL_STR(r.sortie, "0\n"); executer_script_liberer(r);
    r = X("affiche cos(0)\n"); VERIFIER_EGAL_STR(r.sortie, "1\n"); executer_script_liberer(r);
}

static void test_math_abs(void) {
    ResultatExecution r;
    r = X("affiche abs(-7)\n"); VERIFIER_EGAL_STR(r.sortie, "7\n"); executer_script_liberer(r);
    r = X("affiche abs(7)\n"); VERIFIER_EGAL_STR(r.sortie, "7\n"); executer_script_liberer(r);
    r = X("affiche abs(-2.5)\n"); VERIFIER_EGAL_STR(r.sortie, "2.5\n"); executer_script_liberer(r);
}

static void test_math_arrondis(void) {
    ResultatExecution r;
    r = X("affiche plancher(3.9)\n"); VERIFIER_EGAL_STR(r.sortie, "3\n"); executer_script_liberer(r);
    r = X("affiche plafond(3.1)\n"); VERIFIER_EGAL_STR(r.sortie, "4\n"); executer_script_liberer(r);
    r = X("affiche arrondi(3.5)\n"); VERIFIER_EGAL_STR(r.sortie, "4\n"); executer_script_liberer(r);
    r = X("affiche arrondi(3.4)\n"); VERIFIER_EGAL_STR(r.sortie, "3\n"); executer_script_liberer(r);
}

static void test_math_log_exp(void) {
    ResultatExecution r;
    r = X("affiche log(1)\n"); VERIFIER_EGAL_STR(r.sortie, "0\n"); executer_script_liberer(r);
    r = X("affiche exp(0)\n"); VERIFIER_EGAL_STR(r.sortie, "1\n"); executer_script_liberer(r);
    r = X("affiche log(0)\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
}

static void test_math_constantes(void) {
    ResultatExecution r;
    r = X("affiche pi()\n"); VERIFIER_CONTIENT(r.sortie, "3.14159"); executer_script_liberer(r);
    r = X("affiche e()\n"); VERIFIER_CONTIENT(r.sortie, "2.71828"); executer_script_liberer(r);
}

static void test_math_alea(void) {
    ResultatExecution r = X(
        "soit x = alea()\n"
        "affiche x >= 0 et x < 1\n");
    VERIFIER_EGAL_STR(r.sortie, "vrai\n");
    executer_script_liberer(r);

    r = X("affiche alea_entier(5 5)\n");
    VERIFIER_EGAL_STR(r.sortie, "5\n");
    executer_script_liberer(r);
}

static void test_math_min_max(void) {
    ResultatExecution r;
    r = X("affiche min(4 9)\n"); VERIFIER_EGAL_STR(r.sortie, "4\n"); executer_script_liberer(r);
    r = X("affiche max(4 9)\n"); VERIFIER_EGAL_STR(r.sortie, "9\n"); executer_script_liberer(r);
}

static void test_math_mauvais_arguments(void) {
    ResultatExecution r;
    r = X("affiche racine(\"x\")\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
    r = X("affiche racine(1 2)\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
}

/* --- os / fichiers --- */

static void test_os_fichier_cycle_complet(void) {
    char chemin[] = "/tmp/test_natif_os_XXXXXX";
    int fd = mkstemp(chemin);
    close(fd);
    unlink(chemin); /* on veut juste un nom unique, pas encore le fichier */

    char source[512];
    snprintf(source, sizeof(source),
        "affiche fichier_existe(\"%s\")\n"
        "affiche ecrire_fichier(\"%s\" \"bonjour\")\n"
        "affiche fichier_existe(\"%s\")\n"
        "affiche lire_fichier(\"%s\")\n"
        "ajouter_fichier(\"%s\" \" monde\")\n"
        "affiche lire_fichier(\"%s\")\n"
        "affiche supprimer_fichier(\"%s\")\n"
        "affiche fichier_existe(\"%s\")\n"
        "affiche lire_fichier(\"%s\")\n",
        chemin, chemin, chemin, chemin, chemin, chemin, chemin, chemin, chemin);

    ResultatExecution r = X(source);
    VERIFIER_EGAL_STR(r.sortie,
        "faux\nvrai\nvrai\nbonjour\nbonjour monde\nvrai\nfaux\nrien\n");
    executer_script_liberer(r);
}

static void test_os_variable_environnement(void) {
    setenv("EASYLANG_TEST_VAR", "valeur123", 1);
    ResultatExecution r = X("affiche variable_environnement(\"EASYLANG_TEST_VAR\")\n");
    VERIFIER_EGAL_STR(r.sortie, "valeur123\n");
    executer_script_liberer(r);

    r = X("affiche variable_environnement(\"EASYLANG_VAR_ABSENTE_XYZ\")\n");
    VERIFIER_EGAL_STR(r.sortie, "rien\n");
    executer_script_liberer(r);
}

static void test_os_repertoire_courant_non_vide(void) {
    ResultatExecution r = X("affiche longueur(repertoire_courant()) > 0\n");
    VERIFIER_EGAL_STR(r.sortie, "vrai\n");
    executer_script_liberer(r);
}

static void test_os_horodatage_plausible(void) {
    time_t avant = time(NULL);
    ResultatExecution r = X("affiche horodatage()\n");
    time_t apres = time(NULL);
    long valeur = atol(r.sortie);
    VERIFIER(valeur >= (long)avant - 2 && valeur <= (long)apres + 2);
    executer_script_liberer(r);
}

static void test_os_dormir_ne_plante_pas(void) {
    ResultatExecution r = X("dormir(0.01)\naffiche \"termine\"\n");
    VERIFIER_EGAL_STR(r.sortie, "termine\n");
    executer_script_liberer(r);
}

/* --- gui (repli console sur cette plateforme) --- */

static void test_gui_boite_message(void) {
    ResultatExecution r = X("boite_message(\"Titre\" \"Un message\")\n");
    VERIFIER_CONTIENT(r.sortie, "Titre");
    VERIFIER_CONTIENT(r.sortie, "Un message");
    executer_script_liberer(r);
}

static void test_gui_boite_question(void) {
    ResultatExecution r = executer_script("affiche boite_question(\"Q\" \"Ok ?\")\n", "o\n");
    VERIFIER_CONTIENT(r.sortie, "vrai");
    executer_script_liberer(r);

    r = executer_script("affiche boite_question(\"Q\" \"Ok ?\")\n", "n\n");
    VERIFIER_CONTIENT(r.sortie, "faux");
    executer_script_liberer(r);
}

/* --- reseau --- */

static pid_t lancer_serveur_echo(int port) {
    pid_t pid = fork();
    if (pid == 0) {
        int s = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons((uint16_t)port);
        if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) exit(1);
        listen(s, 1);
        int c = accept(s, NULL, NULL);
        char tampon[1024];
        long n = recv(c, tampon, sizeof(tampon) - 1, 0);
        if (n > 0) {
            tampon[n] = '\0';
            char reponse[1100];
            int longueur = snprintf(reponse, sizeof(reponse), "echo: %s", tampon);
            send(c, reponse, (size_t)longueur, 0);
        }
        close(c);
        close(s);
        exit(0);
    }
    usleep(200000);
    return pid;
}

static void test_reseau_aller_retour_complet(void) {
    int port = 19998;
    pid_t serveur = lancer_serveur_echo(port);

    char source[256];
    snprintf(source, sizeof(source),
        "soit s = reseau_connecter(\"127.0.0.1\" %d)\n"
        "affiche s > 0\n"
        "affiche reseau_envoyer(s \"bonjour\")\n"
        "affiche reseau_recevoir(s 1024)\n"
        "reseau_fermer(s)\n",
        port);

    ResultatExecution r = X(source);
    VERIFIER_EGAL_STR(r.sortie, "vrai\n7\necho: bonjour\n");
    executer_script_liberer(r);

    waitpid(serveur, NULL, 0);
}

static void test_reseau_connexion_refusee(void) {
    /* port improbable, rien n'ecoute dessus */
    ResultatExecution r = X("affiche reseau_connecter(\"127.0.0.1\" 1)\n");
    VERIFIER_EGAL_STR(r.sortie, "-1\n");
    executer_script_liberer(r);
}

/* --- bibliotheque (modules d'enveloppe) --- */

static void test_bibliotheque_math(void) {
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"math.elg\" comme math\n"
        "affiche math.racine(16)\n"
        "affiche math.max(3 9)\n"
        "affiche math.PI\n",
        NULL, "../../bibliotheque");
    VERIFIER_EGAL_STR(r.sortie, "4\n9\n3.14159\n");
    executer_script_liberer(r);
}

static void test_bibliotheque_os(void) {
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"os.elg\" comme os\n"
        "affiche longueur(os.repertoire_courant()) > 0\n",
        NULL, "../../bibliotheque");
    VERIFIER_EGAL_STR(r.sortie, "vrai\n");
    executer_script_liberer(r);
}

static void test_bibliotheque_reseau(void) {
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"reseau.elg\" comme reseau\n"
        "affiche reseau.connecter(\"127.0.0.1\" 1)\n",
        NULL, "../../bibliotheque");
    VERIFIER_EGAL_STR(r.sortie, "-1\n");
    executer_script_liberer(r);
}

static void test_bibliotheque_gui(void) {
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"gui.elg\" comme gui\n"
        "gui.message(\"T\" \"M\")\n",
        NULL, "../../bibliotheque");
    VERIFIER_CONTIENT(r.sortie, "M");
    executer_script_liberer(r);
}

void executer_tests_natifs(void) {
    test_math_racine_puissance();
    test_math_racine_negative_erreur();
    test_math_trigo();
    test_math_abs();
    test_math_arrondis();
    test_math_log_exp();
    test_math_constantes();
    test_math_alea();
    test_math_min_max();
    test_math_mauvais_arguments();

    test_os_fichier_cycle_complet();
    test_os_variable_environnement();
    test_os_repertoire_courant_non_vide();
    test_os_horodatage_plausible();
    test_os_dormir_ne_plante_pas();

    test_gui_boite_message();
    test_gui_boite_question();

    test_reseau_aller_retour_complet();
    test_reseau_connexion_refusee();

    test_bibliotheque_math();
    test_bibliotheque_os();
    test_bibliotheque_reseau();
    test_bibliotheque_gui();
}
