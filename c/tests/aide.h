#ifndef AIDE_TESTS_H
#define AIDE_TESTS_H

/* Petit harnais de test maison (aucune dependance externe, coherent avec
   le reste du projet). L'interpreteur fait exit(1) sur toute erreur, donc
   on le pilote en sous-processus (comme une boite noire) via le binaire
   compile ../easylang : c'est la seule facon propre de tester aussi
   les cas d'erreur sans tuer le processus de test lui-meme. */

typedef struct {
    char *sortie;   /* stdout + stderr combines */
    int code_sortie;
} ResultatExecution;

/* Ecrit 'source' dans un fichier .elg temporaire et l'execute avec
   ../easylang. 'entree' (peut etre NULL) est fournie sur stdin. */
ResultatExecution executer_script(const char *source, const char *entree);

/* Comme executer_script, mais le fichier .elg temporaire est cree dans
   'dossier' plutot que /tmp : utile pour tester un 'importe' relatif
   (ex: dossier = "../../bibliotheque" pour importer directement "math.elg"
   sans dependre du chemin absolu du depot). */
ResultatExecution executer_script_dans_dossier(const char *source, const char *entree, const char *dossier);

void executer_script_liberer(ResultatExecution r);

extern int TESTS_TOTAL;
extern int TESTS_ECHOUES;

void verifier_impl(int condition, const char *message, const char *fichier, int ligne);
void verifier_egal_str_impl(const char *obtenu, const char *attendu, const char *fichier, int ligne);
void verifier_contient_impl(const char *obtenu, const char *sous_chaine, const char *fichier, int ligne);

#define VERIFIER(condition) verifier_impl((condition), #condition, __FILE__, __LINE__)
#define VERIFIER_EGAL_STR(obtenu, attendu) verifier_egal_str_impl((obtenu), (attendu), __FILE__, __LINE__)
#define VERIFIER_CONTIENT(obtenu, sous_chaine) verifier_contient_impl((obtenu), (sous_chaine), __FILE__, __LINE__)

#endif
