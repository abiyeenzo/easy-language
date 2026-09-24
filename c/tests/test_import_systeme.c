#include <stdio.h>
#include <string.h>

#include "aide.h"

#define X(source) executer_script((source), NULL)

/* --- importe <nom> (bibliotheque standard, sans chemin) --- */

static void test_import_systeme_math(void) {
    ResultatExecution r = X(
        "importe math\n"
        "affiche math.racine(16)\n"
        "affiche math.max(3 9)\n");
    VERIFIER_EGAL_STR(r.sortie, "4\n9\n");
    executer_script_liberer(r);
}

/* Regression : sans "comme alias", l'alias par defaut (derive du nom du
   fichier) etait autrefois construit dans un tampon de pile local a
   executer_importation. Depuis que environnement_definir emprunte le
   pointeur du nom au lieu de le dupliquer (pour eviter un malloc/free par
   variable liee, v1.0.6), un deuxieme "importe" sans alias reutilisait
   cette meme pile et corrompait silencieusement le nom du premier module
   deja lie (pointeur pendouillant). Corrige en allouant cet alias par
   defaut sur le tas. Ce test garde ce cas precis sous surveillance :
   deux imports sans alias, puis acces au premier. */
static void test_import_systeme_deux_imports_sans_alias(void) {
    ResultatExecution r = X(
        "importe math\n"
        "importe os\n"
        "affiche math.PI\n"
        "affiche longueur(os.repertoire_courant()) > 0\n");
    VERIFIER_CONTIENT(r.sortie, "3.14159");
    VERIFIER_CONTIENT(r.sortie, "vrai");
    executer_script_liberer(r);
}

static void test_import_systeme_avec_alias(void) {
    ResultatExecution r = X(
        "importe texte comme t\n"
        "affiche t.majuscules(\"bonjour\")\n");
    VERIFIER_EGAL_STR(r.sortie, "BONJOUR\n");
    executer_script_liberer(r);
}

static void test_import_systeme_fonctionne_hors_native(void) {
    /* les natifs restent appelables sans prefixe, l'import de module
       n'affecte que l'espace de noms qualifie (voir bibliotheque/temps.elg) */
    ResultatExecution r = X(
        "importe temps\n"
        "affiche temps.annee(0)\n"
        "affiche annee(0)\n");
    VERIFIER_EGAL_STR(r.sortie, "1970\n1970\n");
    executer_script_liberer(r);
}

static void test_import_systeme_inconnu(void) {
    ResultatExecution r = X("importe nimportequoi\n");
    VERIFIER(r.code_sortie != 0);
    VERIFIER_CONTIENT(r.sortie, "bibliotheque standard inconnue");
    executer_script_liberer(r);
}

static void test_import_systeme_chemin_toujours_accepte(void) {
    /* la forme historique "importe "chemin.elg"" doit continuer a
       fonctionner cote de la forme "importe nom" */
    ResultatExecution r = executer_script_dans_dossier(
        "importe \"math.elg\" comme math\n"
        "affiche math.racine(16)\n",
        NULL, "../../bibliotheque");
    VERIFIER_EGAL_STR(r.sortie, "4\n");
    executer_script_liberer(r);
}

/* --- ligne de commande : aide / modules --- */

static char *lancer_cli(const char *arguments) {
    static char tampon[4096];
    char commande[256];
    snprintf(commande, sizeof(commande), "../easy_language %s 2>&1", arguments);
    FILE *p = popen(commande, "r");
    size_t lu = fread(tampon, 1, sizeof(tampon) - 1, p);
    tampon[lu] = '\0';
    pclose(p);
    return tampon;
}

static void test_cli_sans_argument(void) {
    char *sortie = lancer_cli("");
    VERIFIER_CONTIENT(sortie, "Usage");
    VERIFIER_CONTIENT(sortie, "Easy Language");
}

static void test_cli_aide(void) {
    VERIFIER_CONTIENT(lancer_cli("--aide"), "Usage");
    VERIFIER_CONTIENT(lancer_cli("--help"), "Usage");
    VERIFIER_CONTIENT(lancer_cli("-h"), "Usage");
}

static void test_cli_modules(void) {
    char *sortie = lancer_cli("--aide modules");
    VERIFIER_CONTIENT(sortie, "math");
    VERIFIER_CONTIENT(sortie, "os");
    VERIFIER_CONTIENT(sortie, "reseau");
    VERIFIER_CONTIENT(sortie, "gui");
    VERIFIER_CONTIENT(sortie, "texte");
    VERIFIER_CONTIENT(sortie, "temps");

    sortie = lancer_cli("--modules");
    VERIFIER_CONTIENT(sortie, "math");
}

void executer_tests_import_systeme(void) {
    test_import_systeme_math();
    test_import_systeme_deux_imports_sans_alias();
    test_import_systeme_avec_alias();
    test_import_systeme_fonctionne_hors_native();
    test_import_systeme_inconnu();
    test_import_systeme_chemin_toujours_accepte();

    test_cli_sans_argument();
    test_cli_aide();
    test_cli_modules();
}
