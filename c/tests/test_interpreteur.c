#include <stdio.h>
#include <stdlib.h>

#include "aide.h"

void executer_tests_debogueur(void);
void executer_tests_natifs(void);

#define X(source) executer_script((source), NULL)

/* --- variables et arithmetique --- */

static void test_declaration_et_affichage(void) {
    ResultatExecution r = X("soit x = 5\naffiche x\n");
    VERIFIER_EGAL_STR(r.sortie, "5\n");
    executer_script_liberer(r);
}

static void test_reaffectation(void) {
    ResultatExecution r = X("soit x = 1\nx = x + 1\naffiche x\n");
    VERIFIER_EGAL_STR(r.sortie, "2\n");
    executer_script_liberer(r);
}

static void test_operations_arithmetiques(void) {
    ResultatExecution r;
    r = X("affiche 2 + 3 * 4\n"); VERIFIER_EGAL_STR(r.sortie, "14\n"); executer_script_liberer(r);
    r = X("affiche (2 + 3) * 4\n"); VERIFIER_EGAL_STR(r.sortie, "20\n"); executer_script_liberer(r);
    r = X("affiche 7 % 3\n"); VERIFIER_EGAL_STR(r.sortie, "1\n"); executer_script_liberer(r);
    r = X("affiche 7 / 2\n"); VERIFIER_EGAL_STR(r.sortie, "3.5\n"); executer_script_liberer(r);
    r = X("affiche -5 + 2\n"); VERIFIER_EGAL_STR(r.sortie, "-3\n"); executer_script_liberer(r);
}

static void test_concatenation_texte(void) {
    ResultatExecution r;
    r = X("affiche \"a\" + \"b\"\n"); VERIFIER_EGAL_STR(r.sortie, "ab\n"); executer_script_liberer(r);
    r = X("affiche \"valeur:\" + 5\n"); VERIFIER_EGAL_STR(r.sortie, "valeur:5\n"); executer_script_liberer(r);
}

static void test_repetition_texte(void) {
    ResultatExecution r = X("affiche \"ab\" * 3\n");
    VERIFIER_EGAL_STR(r.sortie, "ababab\n");
    executer_script_liberer(r);
}

static void test_division_par_zero(void) {
    ResultatExecution r = X("affiche 1 / 0\n");
    VERIFIER(r.code_sortie != 0);
    VERIFIER_CONTIENT(r.sortie, "division par zero");
    executer_script_liberer(r);
}

static void test_variable_inconnue(void) {
    ResultatExecution r = X("affiche x\n");
    VERIFIER(r.code_sortie != 0);
    VERIFIER_CONTIENT(r.sortie, "variable inconnue");
    executer_script_liberer(r);
}

static void test_affectation_sans_declaration(void) {
    ResultatExecution r = X("x = 1\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

/* --- affichage --- */

static void test_booleens_et_rien(void) {
    ResultatExecution r;
    r = X("affiche vrai\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche faux\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
    r = X("affiche rien\n"); VERIFIER_EGAL_STR(r.sortie, "rien\n"); executer_script_liberer(r);
}

static void test_plusieurs_valeurs(void) {
    ResultatExecution r = X("affiche \"x =\" 5 \"et y =\" 10\n");
    VERIFIER_EGAL_STR(r.sortie, "x = 5 et y = 10\n");
    executer_script_liberer(r);
}

/* --- logique --- */

static void test_comparaisons(void) {
    ResultatExecution r;
    r = X("affiche 5 > 3\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche 5 == 5\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche 5 != 5\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
}

static void test_et_ou_non(void) {
    ResultatExecution r;
    r = X("affiche vrai et faux\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
    r = X("affiche vrai ou faux\n"); VERIFIER_EGAL_STR(r.sortie, "vrai\n"); executer_script_liberer(r);
    r = X("affiche non vrai\n"); VERIFIER_EGAL_STR(r.sortie, "faux\n"); executer_script_liberer(r);
}

static void test_court_circuit_et(void) {
    ResultatExecution r = X(
        "fonction f():\n"
        "    affiche \"appelee\"\n"
        "    retourne vrai\n"
        "affiche faux et f()\n");
    VERIFIER_EGAL_STR(r.sortie, "faux\n");
    executer_script_liberer(r);
}

static void test_court_circuit_ou(void) {
    ResultatExecution r = X(
        "fonction f():\n"
        "    affiche \"appelee\"\n"
        "    retourne faux\n"
        "affiche vrai ou f()\n");
    VERIFIER_EGAL_STR(r.sortie, "vrai\n");
    executer_script_liberer(r);
}

/* --- conditions --- */

static void test_si_vrai(void) {
    ResultatExecution r = X("si vrai alors:\n    affiche \"oui\"\n");
    VERIFIER_EGAL_STR(r.sortie, "oui\n");
    executer_script_liberer(r);
}

static void test_sinon_si(void) {
    ResultatExecution r = X(
        "soit x = 5\n"
        "si x > 10 alors:\n"
        "    affiche \"grand\"\n"
        "sinon si x > 3 alors:\n"
        "    affiche \"moyen\"\n"
        "sinon:\n"
        "    affiche \"petit\"\n");
    VERIFIER_EGAL_STR(r.sortie, "moyen\n");
    executer_script_liberer(r);
}

static void test_sinon(void) {
    ResultatExecution r = X("si faux alors:\n    affiche \"a\"\nsinon:\n    affiche \"b\"\n");
    VERIFIER_EGAL_STR(r.sortie, "b\n");
    executer_script_liberer(r);
}

/* --- boucles --- */

static void test_tantque(void) {
    ResultatExecution r = X("soit i = 0\ntantque i < 3 alors:\n    affiche i\n    i = i + 1\n");
    VERIFIER_EGAL_STR(r.sortie, "0\n1\n2\n");
    executer_script_liberer(r);
}

static void test_pour_de_jusqua(void) {
    ResultatExecution r = X("pour i de 1 jusqua 3 alors:\n    affiche i\n");
    VERIFIER_EGAL_STR(r.sortie, "1\n2\n3\n");
    executer_script_liberer(r);
}

static void test_pour_pas_negatif(void) {
    ResultatExecution r = X("pour i de 3 jusqua 1 pas -1 alors:\n    affiche i\n");
    VERIFIER_EGAL_STR(r.sortie, "3\n2\n1\n");
    executer_script_liberer(r);
}

static void test_pour_pas_zero_interdit(void) {
    ResultatExecution r = X("pour i de 1 jusqua 3 pas 0 alors:\n    affiche i\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_arrete(void) {
    ResultatExecution r = X("pour i de 1 jusqua 5 alors:\n    si i == 3 alors:\n        arrete\n    affiche i\n");
    VERIFIER_EGAL_STR(r.sortie, "1\n2\n");
    executer_script_liberer(r);
}

static void test_continue(void) {
    ResultatExecution r = X("pour i de 1 jusqua 4 alors:\n    si i == 2 alors:\n        continue\n    affiche i\n");
    VERIFIER_EGAL_STR(r.sortie, "1\n3\n4\n");
    executer_script_liberer(r);
}

static void test_pour_chaque_liste(void) {
    ResultatExecution r = X("pour chaque x dans [10 20 30] alors:\n    affiche x\n");
    VERIFIER_EGAL_STR(r.sortie, "10\n20\n30\n");
    executer_script_liberer(r);
}

static void test_pour_chaque_texte(void) {
    ResultatExecution r = X("pour chaque c dans \"ab\" alors:\n    affiche c\n");
    VERIFIER_EGAL_STR(r.sortie, "a\nb\n");
    executer_script_liberer(r);
}

/* --- fonctions --- */

static void test_fonction_simple(void) {
    ResultatExecution r = X("fonction carre(x):\n    retourne x * x\naffiche carre(4)\n");
    VERIFIER_EGAL_STR(r.sortie, "16\n");
    executer_script_liberer(r);
}

static void test_fonction_recursive(void) {
    ResultatExecution r = X(
        "fonction fact(n):\n"
        "    si n <= 1 alors:\n"
        "        retourne 1\n"
        "    retourne n * fact(n - 1)\n"
        "affiche fact(5)\n");
    VERIFIER_EGAL_STR(r.sortie, "120\n");
    executer_script_liberer(r);
}

static void test_fonction_sans_retour_donne_rien(void) {
    ResultatExecution r = X("fonction f():\n    affiche \"salut\"\naffiche f()\n");
    VERIFIER_EGAL_STR(r.sortie, "salut\nrien\n");
    executer_script_liberer(r);
}

static void test_mauvaise_arite(void) {
    ResultatExecution r = X("fonction f(a b):\n    retourne a + b\naffiche f(1)\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_appel_non_fonction(void) {
    ResultatExecution r = X("soit x = 5\naffiche x(1)\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_portee_locale(void) {
    ResultatExecution r = X(
        "soit x = 1\n"
        "fonction f():\n"
        "    soit x = 2\n"
        "    affiche x\n"
        "f()\n"
        "affiche x\n");
    VERIFIER_EGAL_STR(r.sortie, "2\n1\n");
    executer_script_liberer(r);
}

/* --- listes --- */

static void test_liste_litterale_et_affichage(void) {
    ResultatExecution r = X("affiche [1 2 3]\n");
    VERIFIER_EGAL_STR(r.sortie, "[1 2 3]\n");
    executer_script_liberer(r);
}

static void test_indexation_lecture(void) {
    ResultatExecution r = X("soit l = [1 2 3]\naffiche l[1]\n");
    VERIFIER_EGAL_STR(r.sortie, "2\n");
    executer_script_liberer(r);
}

static void test_indexation_ecriture(void) {
    ResultatExecution r = X("soit l = [1 2 3]\nl[0] = 9\naffiche l\n");
    VERIFIER_EGAL_STR(r.sortie, "[9 2 3]\n");
    executer_script_liberer(r);
}

static void test_indexation_hors_limites(void) {
    ResultatExecution r = X("soit l = [1]\naffiche l[5]\n");
    VERIFIER(r.code_sortie != 0);
    VERIFIER_CONTIENT(r.sortie, "index hors limites");
    executer_script_liberer(r);
}

static void test_liste_imbriquee(void) {
    ResultatExecution r = X("soit m = [[1 2] [3 4]]\naffiche m[1][0]\n");
    VERIFIER_EGAL_STR(r.sortie, "3\n");
    executer_script_liberer(r);
}

static void test_ajoute_retire_contient(void) {
    ResultatExecution r = X(
        "soit l = [1 2]\n"
        "ajoute(l 3)\n"
        "affiche l\n"
        "affiche retire(l 0)\n"
        "affiche l\n"
        "affiche contient(l 3)\n"
        "affiche contient(l 99)\n");
    VERIFIER_EGAL_STR(r.sortie, "[1 2 3]\n1\n[2 3]\nvrai\nfaux\n");
    executer_script_liberer(r);
}

static void test_longueur(void) {
    ResultatExecution r = X("affiche longueur([1 2 3 4])\n");
    VERIFIER_EGAL_STR(r.sortie, "4\n");
    executer_script_liberer(r);
}

static void test_liste_vide_est_fausse(void) {
    ResultatExecution r = X("si [] alors:\n    affiche \"vrai\"\nsinon:\n    affiche \"faux\"\n");
    VERIFIER_EGAL_STR(r.sortie, "faux\n");
    executer_script_liberer(r);
}

static void test_concatenation_listes(void) {
    ResultatExecution r = X("affiche [1 2] + [3 4]\n");
    VERIFIER_EGAL_STR(r.sortie, "[1 2 3 4]\n");
    executer_script_liberer(r);
}

/* --- conversions --- */

static void test_conversions(void) {
    ResultatExecution r;
    r = X("affiche nombre(\"42\")\n"); VERIFIER_EGAL_STR(r.sortie, "42\n"); executer_script_liberer(r);
    r = X("affiche entier(3.9)\n"); VERIFIER_EGAL_STR(r.sortie, "3\n"); executer_script_liberer(r);
    r = X("affiche texte(5)\n"); VERIFIER_EGAL_STR(r.sortie, "5\n"); executer_script_liberer(r);
}

/* --- demande (entree) --- */

static void test_demande_conversion_automatique(void) {
    ResultatExecution r = executer_script("demande x\naffiche x + 1\n", "42\n");
    VERIFIER_EGAL_STR(r.sortie, "43\n");
    executer_script_liberer(r);
}

static void test_demande_garde_texte(void) {
    ResultatExecution r = executer_script("demande x\naffiche x + \"!\"\n", "bonjour\n");
    VERIFIER_EGAL_STR(r.sortie, "bonjour!\n");
    executer_script_liberer(r);
}

/* --- erreurs lexicales / syntaxiques --- */

static void test_tabulation_interdite(void) {
    ResultatExecution r = X("si vrai alors:\n\taffiche 1\n");
    VERIFIER(r.code_sortie != 0);
    VERIFIER_CONTIENT(r.sortie, "tabulation");
    executer_script_liberer(r);
}

static void test_alors_manquant(void) {
    ResultatExecution r = X("si vrai:\n    affiche 1\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_deux_points_manquant(void) {
    ResultatExecution r = X("si vrai alors\n    affiche 1\n");
    VERIFIER(r.code_sortie != 0);
    executer_script_liberer(r);
}

static void test_numero_ligne_correct_apres_lignes_vides(void) {
    ResultatExecution r = X("\n\n\naffiche x\n");
    VERIFIER(r.code_sortie != 0);
    VERIFIER_CONTIENT(r.sortie, "ligne 4");
    executer_script_liberer(r);
}

/* --- version --- */

static void test_version(void) {
    FILE *p = popen("../easy_language --version 2>&1", "r");
    char tampon[128] = {0};
    fread(tampon, 1, sizeof(tampon) - 1, p);
    pclose(p);
    VERIFIER_CONTIENT(tampon, "Easy Language");
}

int main(void) {
    test_declaration_et_affichage();
    test_reaffectation();
    test_operations_arithmetiques();
    test_concatenation_texte();
    test_repetition_texte();
    test_division_par_zero();
    test_variable_inconnue();
    test_affectation_sans_declaration();

    test_booleens_et_rien();
    test_plusieurs_valeurs();

    test_comparaisons();
    test_et_ou_non();
    test_court_circuit_et();
    test_court_circuit_ou();

    test_si_vrai();
    test_sinon_si();
    test_sinon();

    test_tantque();
    test_pour_de_jusqua();
    test_pour_pas_negatif();
    test_pour_pas_zero_interdit();
    test_arrete();
    test_continue();
    test_pour_chaque_liste();
    test_pour_chaque_texte();

    test_fonction_simple();
    test_fonction_recursive();
    test_fonction_sans_retour_donne_rien();
    test_mauvaise_arite();
    test_appel_non_fonction();
    test_portee_locale();

    test_liste_litterale_et_affichage();
    test_indexation_lecture();
    test_indexation_ecriture();
    test_indexation_hors_limites();
    test_liste_imbriquee();
    test_ajoute_retire_contient();
    test_longueur();
    test_liste_vide_est_fausse();
    test_concatenation_listes();

    test_conversions();

    test_demande_conversion_automatique();
    test_demande_garde_texte();

    test_tabulation_interdite();
    test_alors_manquant();
    test_deux_points_manquant();
    test_numero_ligne_correct_apres_lignes_vides();

    test_version();

    executer_tests_debogueur();
    executer_tests_natifs();

    printf("%d/%d tests reussis\n", TESTS_TOTAL - TESTS_ECHOUES, TESTS_TOTAL);
    return TESTS_ECHOUES > 0 ? 1 : 0;
}
