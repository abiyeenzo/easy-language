#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *mot; TypeJeton type; } MotCle;

static const MotCle MOTS_CLES[] = {
    {"soit", T_SOIT}, {"affiche", T_AFFICHE}, {"demande", T_DEMANDE},
    {"si", T_SI}, {"alors", T_ALORS}, {"sinon", T_SINON},
    {"tantque", T_TANTQUE}, {"pour", T_POUR}, {"de", T_DE},
    {"jusqua", T_JUSQUA}, {"pas", T_PAS}, {"chaque", T_CHAQUE}, {"dans", T_DANS},
    {"fonction", T_FONCTION}, {"retourne", T_RETOURNE},
    {"vrai", T_VRAI}, {"faux", T_FAUX}, {"rien", T_RIEN},
    {"et", T_ET}, {"ou", T_OU}, {"non", T_NON},
    {"arrete", T_ARRETE}, {"continue", T_CONTINUE},
};
#define NB_MOTS_CLES (sizeof(MOTS_CLES) / sizeof(MOTS_CLES[0]))

static void erreur_lexicale(int ligne, const char *msg) {
    fprintf(stderr, "Erreur ligne %d: %s\n", ligne, msg);
    exit(1);
}

static void ajouter(ListeJetons *l, Jeton j) {
    if (l->compte >= l->capacite) {
        l->capacite = l->capacite ? l->capacite * 2 : 64;
        l->jetons = realloc(l->jetons, sizeof(Jeton) * l->capacite);
    }
    l->jetons[l->compte++] = j;
}

static Jeton jeton_simple(TypeJeton type, int ligne) {
    Jeton j = {0};
    j.type = type;
    j.ligne = ligne;
    return j;
}

typedef struct { const char *sym; TypeJeton type; } Symbole;
static const Symbole SYMBOLES[] = {
    {"==", T_EGAL_EGAL}, {"!=", T_DIFFERENT}, {"<=", T_INF_EGAL}, {">=", T_SUP_EGAL},
    {"+", T_PLUS}, {"-", T_MOINS}, {"*", T_FOIS}, {"/", T_DIVISE}, {"%", T_MODULO},
    {"=", T_EGAL}, {"<", T_INF}, {">", T_SUP},
    {"(", T_PARO}, {")", T_PARF}, {"[", T_CROCHET_O}, {"]", T_CROCHET_F},
    {":", T_DEUX_POINTS}, {",", T_VIRGULE},
};
#define NB_SYMBOLES (sizeof(SYMBOLES) / sizeof(SYMBOLES[0]))

static void tokeniser_contenu(ListeJetons *l, const char *contenu, int ligne) {
    int i = 0;
    int n = (int)strlen(contenu);
    while (i < n) {
        char c = contenu[i];
        if (c == ' ') { i++; continue; }

        if (c == '"') {
            int j = i + 1;
            char tampon[4096];
            int t = 0;
            while (j < n && contenu[j] != '"') {
                if (contenu[j] == '\\' && j + 1 < n) {
                    char suivant = contenu[j + 1];
                    char remplace = suivant;
                    if (suivant == 'n') remplace = '\n';
                    else if (suivant == 't') remplace = '\t';
                    tampon[t++] = remplace;
                    j += 2;
                } else {
                    tampon[t++] = contenu[j++];
                }
            }
            if (j >= n) erreur_lexicale(ligne, "chaine de texte non terminee");
            tampon[t] = '\0';
            Jeton jt = jeton_simple(T_TEXTE, ligne);
            jt.texte = strdup(tampon);
            ajouter(l, jt);
            i = j + 1;
            continue;
        }

        if (isdigit((unsigned char)c)) {
            int j = i;
            int a_point = 0;
            while (j < n && (isdigit((unsigned char)contenu[j]) || (contenu[j] == '.' && !a_point))) {
                if (contenu[j] == '.') a_point = 1;
                j++;
            }
            char brut[64];
            int longueur = j - i;
            memcpy(brut, contenu + i, longueur);
            brut[longueur] = '\0';
            Jeton jt;
            if (a_point) {
                jt = jeton_simple(T_NOMBRE_REEL, ligne);
                jt.reel = atof(brut);
            } else {
                jt = jeton_simple(T_NOMBRE_ENTIER, ligne);
                jt.entier = atoll(brut);
            }
            ajouter(l, jt);
            i = j;
            continue;
        }

        if (isalpha((unsigned char)c) || c == '_') {
            int j = i;
            while (j < n && (isalnum((unsigned char)contenu[j]) || contenu[j] == '_')) j++;
            int longueur = j - i;
            char mot[256];
            memcpy(mot, contenu + i, longueur);
            mot[longueur] = '\0';

            TypeJeton type_trouve = T_IDENT;
            for (size_t k = 0; k < NB_MOTS_CLES; k++) {
                if (strcmp(mot, MOTS_CLES[k].mot) == 0) { type_trouve = MOTS_CLES[k].type; break; }
            }
            Jeton jt = jeton_simple(type_trouve, ligne);
            if (type_trouve == T_IDENT) jt.texte = strdup(mot);
            ajouter(l, jt);
            i = j;
            continue;
        }

        int trouve = 0;
        for (size_t k = 0; k < NB_SYMBOLES; k++) {
            size_t sl = strlen(SYMBOLES[k].sym);
            if (strncmp(contenu + i, SYMBOLES[k].sym, sl) == 0) {
                ajouter(l, jeton_simple(SYMBOLES[k].type, ligne));
                i += (int)sl;
                trouve = 1;
                break;
            }
        }
        if (trouve) continue;

        char msg[64];
        snprintf(msg, sizeof(msg), "caractere inattendu: '%c'", c);
        erreur_lexicale(ligne, msg);
    }
}

ListeJetons lexer_tokeniser(const char *source) {
    ListeJetons resultat = {0};

    int pile_indent[256];
    int profondeur = 0;
    pile_indent[0] = 0;

    const char *curseur = source;
    int numero_ligne = 0;
    int continuer = 1;

    while (continuer) {
        numero_ligne++;

        const char *debut = curseur;
        while (*curseur != '\n' && *curseur != '\0') curseur++;
        int longueur = (int)(curseur - debut);
        if (longueur > 4000) longueur = 4000; /* protege le tampon fixe ci-dessous */
        char ligne_brute[4096];
        memcpy(ligne_brute, debut, longueur);
        ligne_brute[longueur] = '\0';

        if (*curseur == '\0') continuer = 0; else curseur++; /* saute le '\n' */

        /* enlever un eventuel '\r' final (fins de ligne Windows) */
        size_t len = strlen(ligne_brute);
        if (len > 0 && ligne_brute[len - 1] == '\r') ligne_brute[len - 1] = '\0';

        /* verifier tabulations avant le premier '#' */
        for (const char *p = ligne_brute; *p && *p != '#'; p++) {
            if (*p == '\t') erreur_lexicale(numero_ligne, "les tabulations ne sont pas autorisees pour l'indentation, utilisez des espaces");
        }

        int indentation = 0;
        while (ligne_brute[indentation] == ' ') indentation++;

        char contenu[4096];
        strncpy(contenu, ligne_brute + indentation, sizeof(contenu) - 1);
        contenu[sizeof(contenu) - 1] = '\0';
        char *croisillon = strchr(contenu, '#');
        if (croisillon) *croisillon = '\0';
        int fin = (int)strlen(contenu);
        while (fin > 0 && contenu[fin - 1] == ' ') { contenu[--fin] = '\0'; }

        if (contenu[0] == '\0') {
            continue;
        }

        if (indentation > pile_indent[profondeur]) {
            profondeur++;
            pile_indent[profondeur] = indentation;
            ajouter(&resultat, jeton_simple(T_INDENT, numero_ligne));
        } else {
            while (indentation < pile_indent[profondeur]) {
                profondeur--;
                ajouter(&resultat, jeton_simple(T_DEDENT, numero_ligne));
            }
            if (indentation != pile_indent[profondeur]) {
                erreur_lexicale(numero_ligne, "indentation incoherente");
            }
        }

        tokeniser_contenu(&resultat, contenu, numero_ligne);
        ajouter(&resultat, jeton_simple(T_NOUVELLE_LIGNE, numero_ligne));
    }

    while (profondeur > 0) {
        profondeur--;
        ajouter(&resultat, jeton_simple(T_DEDENT, numero_ligne + 1));
    }
    ajouter(&resultat, jeton_simple(T_FIN, numero_ligne + 1));

    return resultat;
}
