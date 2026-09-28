#include <stdlib.h>
#include <time.h>

#include "aide.h"

#define X(source) executer_script((source), NULL)

/* --- texte --- */

static void test_texte_decouper(void) {
    ResultatExecution r;
    r = X("affiche decouper(\"a,b,,c\" \",\")\n");
    VERIFIER_EGAL_STR(r.sortie, "[a b  c]\n");
    executer_script_liberer(r);

    r = X("affiche decouper(\"sans-separateur\" \";\")\n");
    VERIFIER_EGAL_STR(r.sortie, "[sans-separateur]\n");
    executer_script_liberer(r);

    r = X("affiche decouper(\"a\" \"\")\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_texte_joindre(void) {
    ResultatExecution r = X("affiche joindre([\"x\" \"y\" \"z\"] \"-\")\n");
    VERIFIER_EGAL_STR(r.sortie, "x-y-z\n");
    executer_script_liberer(r);

    r = X("affiche joindre([] \"-\")\n");
    VERIFIER_EGAL_STR(r.sortie, "\n");
    executer_script_liberer(r);
}

static void test_texte_remplacer(void) {
    ResultatExecution r;
    r = X("affiche remplacer(\"bonjour le monde\" \"monde\" \"univers\")\n");
    VERIFIER_EGAL_STR(r.sortie, "bonjour le univers\n");
    executer_script_liberer(r);

    r = X("affiche remplacer(\"aaa\" \"a\" \"bb\")\n");
    VERIFIER_EGAL_STR(r.sortie, "bbbbbb\n");
    executer_script_liberer(r);

    r = X("affiche remplacer(\"abc\" \"x\" \"y\")\n");
    VERIFIER_EGAL_STR(r.sortie, "abc\n");
    executer_script_liberer(r);
}

static void test_texte_casse(void) {
    ResultatExecution r;
    r = X("affiche majuscules(\"aBcD\")\n"); VERIFIER_EGAL_STR(r.sortie, "ABCD\n"); executer_script_liberer(r);
    r = X("affiche minuscules(\"aBcD\")\n"); VERIFIER_EGAL_STR(r.sortie, "abcd\n"); executer_script_liberer(r);
}

static void test_texte_rogner(void) {
    ResultatExecution r;
    r = X("affiche rogner(\"   coucou   \")\n"); VERIFIER_EGAL_STR(r.sortie, "coucou\n"); executer_script_liberer(r);
    r = X("affiche rogner(\"deja propre\")\n"); VERIFIER_EGAL_STR(r.sortie, "deja propre\n"); executer_script_liberer(r);
    r = X("affiche rogner(\"   \")\n"); VERIFIER_EGAL_STR(r.sortie, "\n"); executer_script_liberer(r);
}

static void test_texte_prefixe_suffixe(void) {
    ResultatExecution r;
    r = X("affiche commence_par(\"bonjour\" \"bon\")\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche commence_par(\"bonjour\" \"jour\")\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
    r = X("affiche commence_par(\"bon\" \"bonjour\")\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
    r = X("affiche finit_par(\"bonjour\" \"jour\")\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche finit_par(\"bonjour\" \"bon\")\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
}

static void test_texte_sous_texte(void) {
    ResultatExecution r;
    r = X("affiche sous_texte(\"bonjour\" 0 4)\n"); VERIFIER_EGAL_STR(r.sortie, "bonj\n"); executer_script_liberer(r);
    r = X("affiche sous_texte(\"bonjour\" 3 100)\n"); VERIFIER_EGAL_STR(r.sortie, "jour\n"); executer_script_liberer(r);
    r = X("soit d = 0 - 5\naffiche sous_texte(\"bonjour\" d 3)\n"); VERIFIER_EGAL_STR(r.sortie, "bon\n"); executer_script_liberer(r);
    r = X("affiche sous_texte(\"bonjour\" 5 2)\n"); VERIFIER_EGAL_STR(r.sortie, "\n"); executer_script_liberer(r);
}

static void test_texte_inverse(void) {
    ResultatExecution r = X("affiche inverse(\"abcdef\")\n");
    VERIFIER_EGAL_STR(r.sortie, "fedcba\n");
    executer_script_liberer(r);
}

static void test_texte_position_contient(void) {
    ResultatExecution r;
    r = X("affiche position(\"bonjour\" \"jour\")\n"); VERIFIER_EGAL_STR(r.sortie, "3\n"); executer_script_liberer(r);
    r = X("affiche position(\"bonjour\" \"xyz\")\n"); VERIFIER_EGAL_STR(r.sortie, "-1\n"); executer_script_liberer(r);
    r = X("affiche contient_texte(\"bonjour\" \"jour\")\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche contient_texte(\"bonjour\" \"xyz\")\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
}

static void test_texte_mauvais_arguments(void) {
    ResultatExecution r;
    r = X("affiche majuscules(42)\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
    r = X("affiche joindre(\"pas une liste\" \"-\")\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
}

/* --- temps --- */

static void test_temps_maintenant_plausible(void) {
    time_t avant = time(NULL);
    ResultatExecution r = X("affiche maintenant()\n");
    time_t apres = time(NULL);
    long valeur = atol(r.sortie);
    VERIFIER(valeur >= (long)avant - 2 && valeur <= (long)apres + 2);
    executer_script_liberer(r);
}

static void test_temps_composantes_epoque(void) {
    /* horodatage 0 = 1970-01-01 00:00:00 UTC, un jeudi (jour_semaine = 4) */
    ResultatExecution r = X(
        "affiche annee(0)\n"
        "affiche mois(0)\n"
        "affiche jour(0)\n"
        "affiche heure(0)\n"
        "affiche minute(0)\n"
        "affiche seconde(0)\n"
        "affiche jour_semaine(0)\n");
    VERIFIER_EGAL_STR(r.sortie, "1970\n1\n1\n0\n0\n0\n4\n");
    executer_script_liberer(r);
}

static void test_temps_formater(void) {
    ResultatExecution r = X("affiche formater(0 \"%Y-%m-%d %H:%M:%S\")\n");
    VERIFIER_EGAL_STR(r.sortie, "1970-01-01 00:00:00\n");
    executer_script_liberer(r);
}

static void test_temps_mauvais_arguments(void) {
    ResultatExecution r;
    r = X("affiche annee(\"pas un horodatage\")\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
    r = X("affiche formater(0 42)\n"); VERIFIER(r.code_sortie != 0); executer_script_liberer(r);
}

/* --- bibliotheque (modules d'enveloppe) --- */

static void test_bibliotheque_texte(void) {
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"texte.elg\" comme texte\n"
        "affiche texte.majuscules(\"abc\")\n"
        "affiche texte.commence_par(\"bonjour\" \"bon\")\n",
        NULL, "../../bibliotheque");
    VERIFIER_EGAL_STR(r.sortie, "ABC\nvrai\n");
    executer_script_liberer(r);
}

static void test_bibliotheque_temps(void) {
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"temps.elg\" comme temps\n"
        "affiche temps.annee(0)\n"
        "affiche temps.formater(0 \"%Y\")\n",
        NULL, "../../bibliotheque");
    VERIFIER_EGAL_STR(r.sortie, "1970\n1970\n");
    executer_script_liberer(r);
}

void executer_tests_texte_temps(void) {
    setenv("TZ", "UTC", 1);
    tzset();

    test_texte_decouper();
    test_texte_joindre();
    test_texte_remplacer();
    test_texte_casse();
    test_texte_rogner();
    test_texte_prefixe_suffixe();
    test_texte_sous_texte();
    test_texte_inverse();
    test_texte_position_contient();
    test_texte_mauvais_arguments();

    test_temps_maintenant_plausible();
    test_temps_composantes_epoque();
    test_temps_formater();
    test_temps_mauvais_arguments();

    test_bibliotheque_texte();
    test_bibliotheque_temps();
}
