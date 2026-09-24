/* Editeur graphique Easy Language, en Win32 pur (aucune dependance externe
   hors de l'API Windows standard: user32, gdi32, comdlg32, comctl32, et
   Msftedit.dll pour le controle RichEdit fourni avec Windows).

   Fonctionnalites:
   - Edition avec coloration syntaxique basique (mots-cles, chaines,
     nombres, commentaires) via RichEdit + EM_SETCHARFORMAT.
   - Ouvrir / Enregistrer / Enregistrer sous.
   - F5 Lancer: sauvegarde le fichier, lance easylang.exe (suppose
     installe a cote de cet executable) via CreateProcess avec des pipes
     pour recuperer stdout/stderr et fournir une entree standard preecrite.
   - F6 Deboguer: meme principe avec easy_debogueur.exe, mais le pipe
     d'entree reste ouvert pour envoyer des commandes interactives depuis
     un champ de saisie.

   Compile avec: x86_64-w64-mingw32-gcc -o easy_editeur.exe editeur.c
                     -lcomctl32 -lcomdlg32 -lgdi32 -luser32 -lkernel32
*/

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <winhttp.h>

#include "ressources.h"

#define VERSION_EDITEUR "1.0.6"
#define VERSION_EDITEUR_WIDE_(s) L##s
#define VERSION_EDITEUR_WIDE(s) VERSION_EDITEUR_WIDE_(s)
#define VERSION_EDITEUR_L VERSION_EDITEUR_WIDE(VERSION_EDITEUR)

#define ID_EDITEUR 101
#define ID_SORTIE 102
#define ID_ENTREE_COMMANDE 104

#define ID_MENU_NOUVEAU 201
#define ID_MENU_OUVRIR 202
#define ID_MENU_ENREGISTRER 203
#define ID_MENU_ENREGISTRER_SOUS 204
#define ID_MENU_QUITTER 205
#define ID_MENU_LANCER 206
#define ID_MENU_DEBOGUER 207
#define ID_MENU_APROPOS 208
#define ID_MENU_RECHERCHER 209
#define ID_MENU_VERIFIER_MAJ 210
#define ID_MENU_RECENT_EFFACER 298
#define ID_MENU_RECENT_BASE 300
#define MAX_FICHIERS_RECENTS 8

#define WM_APP_SORTIE_TEXTE (WM_APP + 1)
#define WM_APP_PROCESSUS_TERMINE (WM_APP + 2)
#define WM_APP_MAJ_DISPONIBLE (WM_APP + 3)
#define WM_APP_MAJ_AUCUNE (WM_APP + 4)
#define WM_APP_MAJ_ERREUR (WM_APP + 5)

static HWND g_fenetre, g_editeur, g_sortie, g_entree_commande, g_label_entree;
static HWND g_barre_outils, g_barre_etat;
static HWND g_dlg_recherche = NULL;
static UINT g_msg_trouver_prochain = 0;
static FINDREPLACEW g_fr;
static wchar_t g_recherche[256] = L"";
static wchar_t g_chemin_fichier[MAX_PATH] = L"";
static HMENU g_menu_recents = NULL;
static wchar_t g_fichiers_recents[MAX_FICHIERS_RECENTS][MAX_PATH];
static int g_nb_fichiers_recents = 0;
static HANDLE g_pipe_entree_ecriture = NULL;
static HANDLE g_processus_courant = NULL;
static volatile BOOL g_processus_actif = FALSE;
static BOOL g_en_coloration = FALSE;

/* ------------------------------------------------------------------ */
/* coloration syntaxique                                              */

static const wchar_t *MOTS_CLES[] = {
    L"soit", L"affiche", L"demande", L"si", L"alors", L"sinon",
    L"tantque", L"pour", L"de", L"jusqua", L"pas", L"chaque", L"dans",
    L"fonction", L"retourne", L"vrai", L"faux", L"rien",
    L"et", L"ou", L"non", L"arrete", L"continue", L"importe", L"comme",
    NULL,
};

static void definir_couleur(HWND edit, LONG debut, LONG fin, COLORREF couleur) {
    CHARFORMAT2W cf;
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = couleur;
    SendMessageW(edit, EM_SETSEL, (WPARAM)debut, (LPARAM)fin);
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

static int est_caractere_mot(wchar_t c) { return iswalnum(c) || c == L'_'; }

static void colorer_syntaxe(HWND edit) {
    if (g_en_coloration) return;
    g_en_coloration = TRUE;

    int longueur_brut = GetWindowTextLengthW(edit);
    wchar_t *brut = malloc(sizeof(wchar_t) * (longueur_brut + 1));
    GetWindowTextW(edit, brut, longueur_brut + 1);

    /* EM_SETSEL / EM_SETCHARFORMAT comptent chaque saut de paragraphe comme
       un seul caractere ('\r'), alors que GetWindowTextW renvoie des paires
       "\r\n". Sans ce compactage, toute position calculee a partir du
       tampon derive d'une ligne des la deuxieme, et seuls les mots-cles en
       tout debut de fichier semblent correctement colores. */
    wchar_t *texte = malloc(sizeof(wchar_t) * (longueur_brut + 1));
    int longueur = 0;
    for (int j = 0; j < longueur_brut; j++) {
        if (brut[j] != L'\n') texte[longueur++] = brut[j];
    }
    texte[longueur] = L'\0';
    free(brut);

    DWORD sel_debut, sel_fin;
    SendMessageW(edit, EM_GETSEL, (WPARAM)&sel_debut, (LPARAM)&sel_fin);
    SendMessageW(edit, WM_SETREDRAW, FALSE, 0);

    definir_couleur(edit, 0, longueur, RGB(0, 0, 0));

    int i = 0;
    while (i < longueur) {
        wchar_t c = texte[i];

        if (c == L'#') {
            int debut = i;
            while (i < longueur && texte[i] != L'\r') i++;
            definir_couleur(edit, debut, i, RGB(106, 153, 85));
            continue;
        }
        if (c == L'"') {
            int debut = i;
            i++;
            while (i < longueur && texte[i] != L'"' && texte[i] != L'\r') i++;
            if (i < longueur && texte[i] == L'"') i++;
            definir_couleur(edit, debut, i, RGB(206, 145, 120));
            continue;
        }
        if (iswdigit(c)) {
            int debut = i;
            while (i < longueur && (iswdigit(texte[i]) || texte[i] == L'.')) i++;
            definir_couleur(edit, debut, i, RGB(181, 206, 168));
            continue;
        }
        if (iswalpha(c) || c == L'_') {
            int debut = i;
            while (i < longueur && est_caractere_mot(texte[i])) i++;
            int longueur_mot = i - debut;
            for (int k = 0; MOTS_CLES[k]; k++) {
                if ((int)wcslen(MOTS_CLES[k]) == longueur_mot &&
                    wcsncmp(texte + debut, MOTS_CLES[k], longueur_mot) == 0) {
                    definir_couleur(edit, debut, i, RGB(197, 134, 192));
                    break;
                }
            }
            continue;
        }
        i++;
    }

    SendMessageW(edit, EM_SETSEL, sel_debut, sel_fin);
    SendMessageW(edit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(edit, NULL, TRUE);

    free(texte);
    g_en_coloration = FALSE;
}

/* ------------------------------------------------------------------ */
/* sortie / journal                                                   */

static void ajouter_texte_sortie(const wchar_t *texte) {
    int longueur = GetWindowTextLengthW(g_sortie);
    SendMessageW(g_sortie, EM_SETSEL, (WPARAM)longueur, (LPARAM)longueur);
    SendMessageW(g_sortie, EM_REPLACESEL, FALSE, (LPARAM)texte);
}

static void effacer_sortie(void) {
    SetWindowTextW(g_sortie, L"");
}

/* ------------------------------------------------------------------ */
/* barre d'etat                                                       */

static void definir_statut(const wchar_t *texte) {
    SendMessageW(g_barre_etat, SB_SETTEXTW, 0, (LPARAM)texte);
}

static void mettre_a_jour_position_curseur(void) {
    DWORD debut, fin;
    SendMessageW(g_editeur, EM_GETSEL, (WPARAM)&debut, (LPARAM)&fin);
    LRESULT ligne = SendMessageW(g_editeur, EM_LINEFROMCHAR, (WPARAM)debut, 0);
    LRESULT index_ligne = SendMessageW(g_editeur, EM_LINEINDEX, (WPARAM)ligne, 0);
    wchar_t texte[64];
    _snwprintf(texte, 64, L"Ligne %ld, Col %ld", (long)ligne + 1, (long)(debut - index_ligne) + 1);
    SendMessageW(g_barre_etat, SB_SETTEXTW, 1, (LPARAM)texte);
}

/* ------------------------------------------------------------------ */
/* recherche (Ctrl+F)                                                 */

static void rechercher_suivant(const wchar_t *motif) {
    if (!motif || motif[0] == L'\0') return;

    int longueur = GetWindowTextLengthW(g_editeur);
    wchar_t *texte = malloc(sizeof(wchar_t) * (longueur + 1));
    GetWindowTextW(g_editeur, texte, longueur + 1);

    DWORD sel_debut, sel_fin;
    SendMessageW(g_editeur, EM_GETSEL, (WPARAM)&sel_debut, (LPARAM)&sel_fin);

    /* recherche insensible a la casse, en repartant apres la selection
       courante, avec retour au debut si rien n'est trouve plus loin */
    wchar_t *texte_bas = _wcsdup(texte);
    wchar_t *motif_bas = _wcsdup(motif);
    _wcslwr(texte_bas);
    _wcslwr(motif_bas);

    wchar_t *trouve = wcsstr(texte_bas + sel_fin, motif_bas);
    int position = -1;
    if (trouve) {
        position = (int)(trouve - texte_bas);
    } else {
        trouve = wcsstr(texte_bas, motif_bas);
        if (trouve) position = (int)(trouve - texte_bas);
    }

    if (position >= 0) {
        int fin_trouve = position + (int)wcslen(motif);
        SendMessageW(g_editeur, EM_SETSEL, (WPARAM)position, (LPARAM)fin_trouve);
        SendMessageW(g_editeur, EM_SCROLLCARET, 0, 0);
        SetFocus(g_editeur);
    } else {
        MessageBoxW(g_fenetre, L"Texte introuvable.", L"Rechercher", MB_ICONINFORMATION);
    }

    free(texte_bas);
    free(motif_bas);
    free(texte);
}

static void ouvrir_recherche(void) {
    if (g_dlg_recherche) {
        SetFocus(g_dlg_recherche);
        return;
    }
    memset(&g_fr, 0, sizeof(g_fr));
    g_fr.lStructSize = sizeof(g_fr);
    g_fr.hwndOwner = g_fenetre;
    g_fr.lpstrFindWhat = g_recherche;
    g_fr.wFindWhatLen = sizeof(g_recherche) / sizeof(wchar_t);
    g_fr.Flags = FR_DOWN;
    g_dlg_recherche = FindTextW(&g_fr);
}

/* ------------------------------------------------------------------ */
/* verification de mise a jour (GitHub Releases)                      */

/* Requete HTTPS GET minimale via WinHTTP (deja presente sur Windows,
   aucune dependance a ajouter). Retourne le corps de la reponse
   (chaine C allouee, a liberer par l'appelant) ou NULL en cas d'echec
   (pas de connexion, DNS, etc.) : jamais d'erreur bloquante pour une
   verification automatique au demarrage. */
static char *http_get(const wchar_t *hote, const wchar_t *chemin) {
    char *resultat = NULL;
    HINTERNET session = WinHttpOpen(L"EasyLanguageEditeur/" VERSION_EDITEUR_L,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return NULL;

    HINTERNET connexion = WinHttpConnect(session, hote, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connexion) { WinHttpCloseHandle(session); return NULL; }

    HINTERNET requete = WinHttpOpenRequest(connexion, L"GET", chemin, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!requete) { WinHttpCloseHandle(connexion); WinHttpCloseHandle(session); return NULL; }

    BOOL ok = WinHttpSendRequest(requete, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok) ok = WinHttpReceiveResponse(requete, NULL);

    if (ok) {
        size_t capacite = 4096, taille = 0;
        resultat = malloc(capacite);
        DWORD disponible;
        while (WinHttpQueryDataAvailable(requete, &disponible) && disponible > 0) {
            if (taille + disponible + 1 > capacite) {
                while (taille + disponible + 1 > capacite) capacite *= 2;
                resultat = realloc(resultat, capacite);
            }
            DWORD lu;
            if (!WinHttpReadData(requete, resultat + taille, disponible, &lu)) break;
            taille += lu;
        }
        resultat[taille] = '\0';
    }

    WinHttpCloseHandle(requete);
    WinHttpCloseHandle(connexion);
    WinHttpCloseHandle(session);
    return resultat;
}

/* Extrait la valeur de "tag_name":"..." d'une reponse JSON de l'API
   GitHub Releases. Recherche textuelle ciblee plutot qu'un analyseur
   JSON complet : suffisant et fiable pour ce seul champ, sur un format
   de reponse stable et documente. */
static BOOL extraire_tag(const char *json, char *dehors, size_t taille) {
    const char *cle = "\"tag_name\":\"";
    const char *debut = strstr(json, cle);
    if (!debut) return FALSE;
    debut += strlen(cle);
    const char *fin = strchr(debut, '"');
    if (!fin) return FALSE;
    size_t longueur = (size_t)(fin - debut);
    if (longueur >= taille) longueur = taille - 1;
    memcpy(dehors, debut, longueur);
    dehors[longueur] = '\0';
    return TRUE;
}

/* Compare une version "vX.Y.Z" (ou "X.Y.Z") de release a VERSION_EDITEUR.
   Retourne 1 si distante > locale, 0 sinon (egale, plus ancienne, ou
   format illisible). */
static int version_plus_recente(const char *tag) {
    if (tag[0] == 'v' || tag[0] == 'V') tag++;
    int ma = 0, mi = 0, pa = 0;
    int la = 0, li = 0, lo = 0;
    if (sscanf(tag, "%d.%d.%d", &ma, &mi, &pa) != 3) return 0;
    if (sscanf(VERSION_EDITEUR, "%d.%d.%d", &la, &li, &lo) != 3) return 0;
    if (ma != la) return ma > la;
    if (mi != li) return mi > li;
    return pa > lo;
}

static DWORD WINAPI thread_verifier_maj(LPVOID param) {
    BOOL silencieux = (BOOL)(INT_PTR)param;
    char *reponse = http_get(L"api.github.com", L"/repos/abiyeenzo/easy-language/releases/latest");
    if (!reponse) {
        if (!silencieux) PostMessageW(g_fenetre, WM_APP_MAJ_ERREUR, 0, 0);
        return 0;
    }

    char tag[64];
    BOOL trouve = extraire_tag(reponse, tag, sizeof(tag));
    free(reponse);

    if (trouve && version_plus_recente(tag)) {
        int longueur_large = MultiByteToWideChar(CP_UTF8, 0, tag, -1, NULL, 0);
        wchar_t *large = malloc(sizeof(wchar_t) * longueur_large);
        MultiByteToWideChar(CP_UTF8, 0, tag, -1, large, longueur_large);
        PostMessageW(g_fenetre, WM_APP_MAJ_DISPONIBLE, 0, (LPARAM)large);
    } else if (!silencieux) {
        PostMessageW(g_fenetre, WM_APP_MAJ_AUCUNE, 0, 0);
    }
    return 0;
}

static void lancer_verification_maj(BOOL silencieux) {
    CloseHandle(CreateThread(NULL, 0, thread_verifier_maj, (LPVOID)(INT_PTR)silencieux, 0, NULL));
}

/* ------------------------------------------------------------------ */
/* lancement de processus (interpreteur / debogueur) avec pipes       */

typedef struct {
    HANDLE pipe_lecture;
    HANDLE processus;
} ParametresThread;

static DWORD WINAPI thread_lecture_sortie(LPVOID param) {
    ParametresThread *p = (ParametresThread *)param;
    char tampon[4096];
    DWORD lu;
    while (ReadFile(p->pipe_lecture, tampon, sizeof(tampon) - 1, &lu, NULL) && lu > 0) {
        tampon[lu] = '\0';
        int longueur_large = MultiByteToWideChar(CP_UTF8, 0, tampon, -1, NULL, 0);
        wchar_t *large = malloc(sizeof(wchar_t) * longueur_large);
        MultiByteToWideChar(CP_UTF8, 0, tampon, -1, large, longueur_large);
        PostMessageW(g_fenetre, WM_APP_SORTIE_TEXTE, 0, (LPARAM)large);
    }
    CloseHandle(p->pipe_lecture);
    WaitForSingleObject(p->processus, INFINITE);
    DWORD code_sortie = 0;
    GetExitCodeProcess(p->processus, &code_sortie);
    CloseHandle(p->processus);
    PostMessageW(g_fenetre, WM_APP_PROCESSUS_TERMINE, (WPARAM)code_sortie, 0);
    free(p);
    return 0;
}

static BOOL obtenir_chemin_frere(const wchar_t *nom_exe, wchar_t *dehors, DWORD taille) {
    wchar_t chemin_soi[MAX_PATH];
    if (!GetModuleFileNameW(NULL, chemin_soi, MAX_PATH)) return FALSE;
    wchar_t *dernier_slash = wcsrchr(chemin_soi, L'\\');
    if (!dernier_slash) return FALSE;
    *(dernier_slash + 1) = L'\0';
    _snwprintf(dehors, taille, L"%s%s", chemin_soi, nom_exe);
    return TRUE;
}

static void ecrire_utf8(HANDLE pipe, const wchar_t *texte) {
    int taille_utf8 = WideCharToMultiByte(CP_UTF8, 0, texte, -1, NULL, 0, NULL, NULL);
    char *utf8 = malloc(taille_utf8);
    WideCharToMultiByte(CP_UTF8, 0, texte, -1, utf8, taille_utf8, NULL, NULL);
    DWORD ecrit;
    WriteFile(pipe, utf8, (DWORD)strlen(utf8), &ecrit, NULL);
    free(utf8);
}

static void lancer_processus(const wchar_t *nom_exe, const wchar_t *argument_supplementaire, BOOL mode_debogueur) {
    if (g_processus_actif) {
        MessageBoxW(g_fenetre, L"Une execution est deja en cours.", L"Easy Language", MB_ICONINFORMATION);
        return;
    }

    wchar_t chemin_exe[MAX_PATH];
    if (!obtenir_chemin_frere(nom_exe, chemin_exe, MAX_PATH)) {
        MessageBoxW(g_fenetre, L"Impossible de localiser l'executable.", L"Erreur", MB_ICONERROR);
        definir_statut(L"Erreur: executable introuvable");
        return;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE sortie_lecture, sortie_ecriture;
    CreatePipe(&sortie_lecture, &sortie_ecriture, &sa, 0);
    SetHandleInformation(sortie_lecture, HANDLE_FLAG_INHERIT, 0);

    HANDLE entree_lecture, entree_ecriture;
    CreatePipe(&entree_lecture, &entree_ecriture, &sa, 0);
    SetHandleInformation(entree_ecriture, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = sortie_ecriture;
    si.hStdError = sortie_ecriture;
    si.hStdInput = entree_lecture;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    wchar_t ligne_commande[4096];
    if (argument_supplementaire) {
        _snwprintf(ligne_commande, 4096, L"\"%s\" \"%s\" %s", chemin_exe, g_chemin_fichier, argument_supplementaire);
    } else {
        _snwprintf(ligne_commande, 4096, L"\"%s\" \"%s\"", chemin_exe, g_chemin_fichier);
    }

    BOOL ok = CreateProcessW(NULL, ligne_commande, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(sortie_ecriture);
    CloseHandle(entree_lecture);

    if (!ok) {
        MessageBoxW(g_fenetre, L"Impossible de lancer l'executable (est-il bien installe a cote de l'editeur ?)", L"Erreur", MB_ICONERROR);
        definir_statut(L"Erreur: lancement impossible");
        CloseHandle(sortie_lecture);
        CloseHandle(entree_ecriture);
        return;
    }

    definir_statut(L"Execution en cours...");

    /* Le pipe d'entree reste ouvert dans les deux modes: l'utilisateur
       tape une ligne dans g_entree_commande et l'envoie avec Entree des
       qu'un 'demande' (ou une commande de debogueur) en a besoin, au lieu
       de devoir tout pre-remplir avant le lancement. */
    (void)mode_debogueur;
    g_pipe_entree_ecriture = entree_ecriture;
    EnableWindow(g_entree_commande, TRUE);
    SetFocus(g_entree_commande);

    g_processus_courant = pi.hProcess;
    g_processus_actif = TRUE;

    ParametresThread *params = malloc(sizeof(ParametresThread));
    params->pipe_lecture = sortie_lecture;
    params->processus = pi.hProcess;
    CloseHandle(CreateThread(NULL, 0, thread_lecture_sortie, params, 0, NULL));

    CloseHandle(pi.hThread);
}

/* ------------------------------------------------------------------ */
/* fichiers recents (persistes dans le registre HKCU, par utilisateur) */

#define CLE_REGISTRE_RECENTS L"Software\\EasyLanguage\\Editeur"
#define VALEUR_REGISTRE_RECENTS L"FichiersRecents"

static void charger_fichiers_recents(void) {
    g_nb_fichiers_recents = 0;
    HKEY cle;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, CLE_REGISTRE_RECENTS, 0, KEY_READ, &cle) != ERROR_SUCCESS) return;

    wchar_t tampon[MAX_FICHIERS_RECENTS * MAX_PATH];
    DWORD taille = sizeof(tampon);
    DWORD type;
    if (RegQueryValueExW(cle, VALEUR_REGISTRE_RECENTS, NULL, &type, (LPBYTE)tampon, &taille) == ERROR_SUCCESS && type == REG_MULTI_SZ) {
        wchar_t *p = tampon;
        while (*p && g_nb_fichiers_recents < MAX_FICHIERS_RECENTS) {
            wcsncpy(g_fichiers_recents[g_nb_fichiers_recents], p, MAX_PATH - 1);
            g_fichiers_recents[g_nb_fichiers_recents][MAX_PATH - 1] = L'\0';
            g_nb_fichiers_recents++;
            p += wcslen(p) + 1;
        }
    }
    RegCloseKey(cle);
}

static void sauvegarder_fichiers_recents(void) {
    HKEY cle;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, CLE_REGISTRE_RECENTS, 0, NULL, 0, KEY_WRITE, NULL, &cle, NULL) != ERROR_SUCCESS) return;

    /* format REG_MULTI_SZ : chaines terminees par '\0', l'ensemble termine par un '\0' supplementaire */
    wchar_t tampon[MAX_FICHIERS_RECENTS * MAX_PATH];
    size_t position = 0;
    for (int i = 0; i < g_nb_fichiers_recents; i++) {
        size_t longueur = wcslen(g_fichiers_recents[i]) + 1;
        memcpy(tampon + position, g_fichiers_recents[i], longueur * sizeof(wchar_t));
        position += longueur;
    }
    tampon[position++] = L'\0';

    RegSetValueExW(cle, VALEUR_REGISTRE_RECENTS, 0, REG_MULTI_SZ, (const BYTE *)tampon, (DWORD)(position * sizeof(wchar_t)));
    RegCloseKey(cle);
}

static void reconstruire_menu_recents(void) {
    if (!g_menu_recents) return;
    while (GetMenuItemCount(g_menu_recents) > 0) {
        RemoveMenu(g_menu_recents, 0, MF_BYPOSITION);
    }

    if (g_nb_fichiers_recents == 0) {
        AppendMenuW(g_menu_recents, MF_STRING | MF_GRAYED, 0, L"(aucun)");
        return;
    }

    for (int i = 0; i < g_nb_fichiers_recents; i++) {
        const wchar_t *nom_fichier = wcsrchr(g_fichiers_recents[i], L'\\');
        nom_fichier = nom_fichier ? nom_fichier + 1 : g_fichiers_recents[i];
        wchar_t etiquette[MAX_PATH + 8];
        _snwprintf(etiquette, MAX_PATH + 8, L"&%d %s", i + 1, nom_fichier);
        AppendMenuW(g_menu_recents, MF_STRING, ID_MENU_RECENT_BASE + i, etiquette);
    }
    AppendMenuW(g_menu_recents, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_menu_recents, MF_STRING, ID_MENU_RECENT_EFFACER, L"Effacer la liste");
}

static void ajouter_fichier_recent(const wchar_t *chemin) {
    /* retire toute entree existante pour ce chemin (comparaison sans la
       casse : les chemins Windows ne sont pas sensibles a la casse), pour
       la remonter en tete plutot que la dupliquer */
    int existant = -1;
    for (int i = 0; i < g_nb_fichiers_recents; i++) {
        if (_wcsicmp(g_fichiers_recents[i], chemin) == 0) { existant = i; break; }
    }
    int fin = (existant >= 0) ? existant : (g_nb_fichiers_recents < MAX_FICHIERS_RECENTS ? g_nb_fichiers_recents : MAX_FICHIERS_RECENTS - 1);
    for (int i = fin; i > 0; i--) {
        wcsncpy(g_fichiers_recents[i], g_fichiers_recents[i - 1], MAX_PATH);
    }
    wcsncpy(g_fichiers_recents[0], chemin, MAX_PATH - 1);
    g_fichiers_recents[0][MAX_PATH - 1] = L'\0';
    if (existant < 0 && g_nb_fichiers_recents < MAX_FICHIERS_RECENTS) g_nb_fichiers_recents++;

    sauvegarder_fichiers_recents();
    reconstruire_menu_recents();
}

static void effacer_fichiers_recents(void) {
    g_nb_fichiers_recents = 0;
    sauvegarder_fichiers_recents();
    reconstruire_menu_recents();
}

/* Charge le contenu d'un fichier .elg dans l'editeur. Partagee par
   "Ouvrir..." (boite de dialogue standard) et le menu "Fichiers recents". */
static BOOL charger_fichier(const wchar_t *chemin) {
    HANDLE f = CreateFileW(chemin, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return FALSE;
    DWORD taille = GetFileSize(f, NULL);
    char *utf8 = malloc(taille + 1);
    DWORD lu;
    ReadFile(f, utf8, taille, &lu, NULL);
    utf8[lu] = '\0';
    CloseHandle(f);

    int longueur_large = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wchar_t *large = malloc(sizeof(wchar_t) * longueur_large);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, large, longueur_large);
    SetWindowTextW(g_editeur, large);
    free(utf8);
    free(large);

    wcsncpy(g_chemin_fichier, chemin, MAX_PATH - 1);
    g_chemin_fichier[MAX_PATH - 1] = L'\0';
    colorer_syntaxe(g_editeur);
    ajouter_fichier_recent(chemin);
    return TRUE;
}

static BOOL enregistrer_fichier(void) {
    if (g_chemin_fichier[0] == L'\0') return FALSE;

    int longueur = GetWindowTextLengthW(g_editeur);
    wchar_t *texte = malloc(sizeof(wchar_t) * (longueur + 1));
    GetWindowTextW(g_editeur, texte, longueur + 1);

    int taille_utf8 = WideCharToMultiByte(CP_UTF8, 0, texte, -1, NULL, 0, NULL, NULL);
    char *utf8 = malloc(taille_utf8);
    WideCharToMultiByte(CP_UTF8, 0, texte, -1, utf8, taille_utf8, NULL, NULL);

    HANDLE f = CreateFileW(g_chemin_fichier, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD ecrit;
        WriteFile(f, utf8, (DWORD)strlen(utf8), &ecrit, NULL);
        CloseHandle(f);
    }
    free(texte);
    free(utf8);
    return TRUE;
}

static BOOL enregistrer_sous(void) {
    wchar_t tampon[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_fenetre;
    ofn.lpstrFilter = L"Easy Language (*.elg)\0*.elg\0Tous les fichiers\0*.*\0";
    ofn.lpstrFile = tampon;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"elg";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn)) return FALSE;
    wcsncpy(g_chemin_fichier, tampon, MAX_PATH);
    BOOL ok = enregistrer_fichier();
    if (ok) ajouter_fichier_recent(g_chemin_fichier);
    return ok;
}

static void ouvrir_fichier(void) {
    wchar_t tampon[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_fenetre;
    ofn.lpstrFilter = L"Easy Language (*.elg)\0*.elg\0Tous les fichiers\0*.*\0";
    ofn.lpstrFile = tampon;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameW(&ofn)) return;
    if (!charger_fichier(tampon)) {
        MessageBoxW(g_fenetre, L"Impossible d'ouvrir ce fichier.", L"Erreur", MB_ICONERROR);
    }
}

/* ------------------------------------------------------------------ */
/* interface                                                          */

static void creer_menu(HWND hwnd) {
    HMENU barre = CreateMenu();

    HMENU menu_fichier = CreatePopupMenu();
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_NOUVEAU, L"Nouveau\tCtrl+N");
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_OUVRIR, L"Ouvrir...\tCtrl+O");
    g_menu_recents = CreatePopupMenu();
    AppendMenuW(menu_fichier, MF_POPUP, (UINT_PTR)g_menu_recents, L"Fichiers recents");
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_ENREGISTRER, L"Enregistrer\tCtrl+S");
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_ENREGISTRER_SOUS, L"Enregistrer sous...");
    AppendMenuW(menu_fichier, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_QUITTER, L"Quitter");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_fichier, L"Fichier");

    HMENU menu_edition = CreatePopupMenu();
    AppendMenuW(menu_edition, MF_STRING, ID_MENU_RECHERCHER, L"Rechercher...\tCtrl+F");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_edition, L"Edition");

    HMENU menu_executer = CreatePopupMenu();
    AppendMenuW(menu_executer, MF_STRING, ID_MENU_LANCER, L"Lancer\tF5");
    AppendMenuW(menu_executer, MF_STRING, ID_MENU_DEBOGUER, L"Deboguer\tF6");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_executer, L"Executer");

    HMENU menu_aide = CreatePopupMenu();
    AppendMenuW(menu_aide, MF_STRING, ID_MENU_VERIFIER_MAJ, L"Verifier les mises a jour");
    AppendMenuW(menu_aide, MF_STRING, ID_MENU_APROPOS, L"A propos");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_aide, L"Aide");

    SetMenu(hwnd, barre);
}

static void creer_barre_outils(HWND hwnd) {
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    g_barre_outils = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NODIVIDER,
        0, 0, 0, 0, hwnd, NULL, instance, NULL);
    SendMessageW(g_barre_outils, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON boutons[] = {
        { I_IMAGENONE, ID_MENU_NOUVEAU, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0}, 0, (INT_PTR)L"Nouveau" },
        { I_IMAGENONE, ID_MENU_OUVRIR, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0}, 0, (INT_PTR)L"Ouvrir" },
        { I_IMAGENONE, ID_MENU_ENREGISTRER, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0}, 0, (INT_PTR)L"Enregistrer" },
        { I_IMAGENONE, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_MENU_LANCER, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0}, 0, (INT_PTR)L"Lancer" },
        { I_IMAGENONE, ID_MENU_DEBOGUER, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0}, 0, (INT_PTR)L"Deboguer" },
        { I_IMAGENONE, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_MENU_RECHERCHER, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0}, 0, (INT_PTR)L"Rechercher" },
    };
    SendMessageW(g_barre_outils, TB_ADDBUTTONSW, sizeof(boutons) / sizeof(boutons[0]), (LPARAM)boutons);
    SendMessageW(g_barre_outils, TB_AUTOSIZE, 0, 0);
}

static void creer_barre_etat(HWND hwnd) {
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    g_barre_etat = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hwnd, NULL, instance, NULL);
    int parties[] = { 220, -1 };
    SendMessageW(g_barre_etat, SB_SETPARTS, 2, (LPARAM)parties);
    definir_statut(L"Pret");
    SendMessageW(g_barre_etat, SB_SETTEXTW, 1, (LPARAM)L"Ligne 1, Col 1");
}

static void creer_controles(HWND hwnd) {
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    HFONT police = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                FIXED_PITCH, L"Consolas");

    creer_barre_outils(hwnd);
    creer_barre_etat(hwnd);

    g_editeur = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        0, 0, 0, 0, hwnd, (HMENU)ID_EDITEUR, instance, NULL);
    SendMessageW(g_editeur, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
    SendMessageW(g_editeur, WM_SETFONT, (WPARAM)police, TRUE);

    CreateWindowExW(0, L"STATIC", L"Sortie:", WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, hwnd, NULL, instance, NULL);

    g_sortie = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0, hwnd, (HMENU)ID_SORTIE, instance, NULL);
    SendMessageW(g_sortie, WM_SETFONT, (WPARAM)police, TRUE);

    g_label_entree = CreateWindowExW(0, L"STATIC",
        L"Entree (tapez une reponse pour 'demande', ou une commande de debogueur, puis Entree):",
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, instance, NULL);

    g_entree_commande = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_DISABLED,
        0, 0, 0, 0, hwnd, (HMENU)ID_ENTREE_COMMANDE, instance, NULL);
    SendMessageW(g_entree_commande, WM_SETFONT, (WPARAM)police, TRUE);
}

static void redimensionner_controles(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int largeur = rc.right - rc.left;
    int hauteur = rc.bottom - rc.top;

    SendMessageW(g_barre_outils, TB_AUTOSIZE, 0, 0);
    RECT rc_outils;
    GetWindowRect(g_barre_outils, &rc_outils);
    int h_outils = rc_outils.bottom - rc_outils.top;
    MoveWindow(g_barre_outils, 0, 0, largeur, h_outils, TRUE);

    SendMessageW(g_barre_etat, WM_SIZE, 0, 0);
    RECT rc_etat;
    GetWindowRect(g_barre_etat, &rc_etat);
    int h_etat = rc_etat.bottom - rc_etat.top;

    int y = h_outils;
    int zone_utile = hauteur - h_outils - h_etat;
    int h_editeur = (int)(zone_utile * 0.6);
    MoveWindow(g_editeur, 0, y, largeur, h_editeur, TRUE);
    y += h_editeur;

    int h_sortie = zone_utile - h_editeur - 44;
    if (h_sortie < 50) h_sortie = 50;
    MoveWindow(g_sortie, 0, y, largeur, h_sortie, TRUE);
    y += h_sortie;

    MoveWindow(g_label_entree, 0, y, largeur, 18, TRUE);
    y += 18;
    MoveWindow(g_entree_commande, 0, y, largeur, 24, TRUE);
}

static void mettre_a_jour_titre(HWND hwnd) {
    wchar_t titre[MAX_PATH + 64];
    if (g_chemin_fichier[0] == L'\0') {
        wcscpy(titre, L"Easy Language " VERSION_EDITEUR_L L" - Editeur [Nouveau fichier]");
    } else {
        _snwprintf(titre, MAX_PATH + 64, L"Easy Language " VERSION_EDITEUR_L L" - Editeur [%s]", g_chemin_fichier);
    }
    SetWindowTextW(hwnd, titre);
}

static void gerer_commande(HWND hwnd, WPARAM wp, LPARAM lp) {
    int id = LOWORD(wp);
    int notification = HIWORD(wp);

    if ((HWND)lp == g_editeur && notification == EN_CHANGE) {
        colorer_syntaxe(g_editeur);
        mettre_a_jour_position_curseur();
        return;
    }
    if ((HWND)lp == g_editeur && notification == EN_SELCHANGE) {
        mettre_a_jour_position_curseur();
        return;
    }

    if (id >= ID_MENU_RECENT_BASE && id < ID_MENU_RECENT_BASE + MAX_FICHIERS_RECENTS) {
        int index = id - ID_MENU_RECENT_BASE;
        if (index < g_nb_fichiers_recents) {
            if (!charger_fichier(g_fichiers_recents[index])) {
                MessageBoxW(hwnd, L"Ce fichier n'existe plus, retire de la liste.", L"Fichiers recents", MB_ICONWARNING);
                for (int i = index; i < g_nb_fichiers_recents - 1; i++) {
                    wcsncpy(g_fichiers_recents[i], g_fichiers_recents[i + 1], MAX_PATH);
                }
                g_nb_fichiers_recents--;
                sauvegarder_fichiers_recents();
                reconstruire_menu_recents();
            } else {
                mettre_a_jour_titre(hwnd);
                definir_statut(L"Pret");
            }
        }
        return;
    }

    switch (id) {
        case ID_MENU_NOUVEAU:
            SetWindowTextW(g_editeur, L"");
            g_chemin_fichier[0] = L'\0';
            mettre_a_jour_titre(hwnd);
            definir_statut(L"Pret");
            break;
        case ID_MENU_OUVRIR:
            ouvrir_fichier();
            mettre_a_jour_titre(hwnd);
            definir_statut(L"Pret");
            break;
        case ID_MENU_RECHERCHER:
            ouvrir_recherche();
            break;
        case ID_MENU_ENREGISTRER:
            if (!enregistrer_fichier()) { if (enregistrer_sous()) mettre_a_jour_titre(hwnd); }
            break;
        case ID_MENU_ENREGISTRER_SOUS:
            if (enregistrer_sous()) mettre_a_jour_titre(hwnd);
            break;
        case ID_MENU_QUITTER:
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            break;
        case ID_MENU_LANCER:
            if (g_chemin_fichier[0] == L'\0' && !enregistrer_sous()) break;
            enregistrer_fichier();
            mettre_a_jour_titre(hwnd);
            effacer_sortie();
            lancer_processus(L"easylang.exe", NULL, FALSE);
            break;
        case ID_MENU_DEBOGUER:
            if (g_chemin_fichier[0] == L'\0' && !enregistrer_sous()) break;
            enregistrer_fichier();
            mettre_a_jour_titre(hwnd);
            effacer_sortie();
            lancer_processus(L"easy_debogueur.exe", NULL, TRUE);
            break;
        case ID_MENU_APROPOS:
            MessageBoxW(hwnd,
                L"Easy Language " VERSION_EDITEUR_L L"\n\nLangage de programmation interprete en francais.\nFichiers .elg\n\nhttps://github.com/abiyeenzo/easy-language",
                L"A propos", MB_OK);
            break;
        case ID_MENU_VERIFIER_MAJ:
            lancer_verification_maj(FALSE);
            break;
        case ID_MENU_RECENT_EFFACER:
            effacer_fichiers_recents();
            break;
        case ID_ENTREE_COMMANDE:
            if (notification == EN_CHANGE) break;
            break;
    }
}

static void envoyer_ligne_entree(void) {
    if (!g_pipe_entree_ecriture) return;
    int longueur = GetWindowTextLengthW(g_entree_commande);
    wchar_t *texte = malloc(sizeof(wchar_t) * (longueur + 2));
    GetWindowTextW(g_entree_commande, texte, longueur + 1);
    wcscat(texte, L"\r\n");

    wchar_t echo[600];
    _snwprintf(echo, 600, L"> %s\r\n", texte);
    ajouter_texte_sortie(echo);

    ecrire_utf8(g_pipe_entree_ecriture, texte);
    free(texte);
    SetWindowTextW(g_entree_commande, L"");
}

static WNDPROC g_proc_originale_commande;
static LRESULT CALLBACK proc_entree_commande(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN && wp == VK_RETURN) {
        envoyer_ligne_entree();
        return 0;
    }
    return CallWindowProcW(g_proc_originale_commande, hwnd, msg, wp, lp);
}

LRESULT CALLBACK FenetrePrincipaleProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            creer_menu(hwnd);
            creer_controles(hwnd);
            g_proc_originale_commande = (WNDPROC)SetWindowLongPtrW(g_entree_commande, GWLP_WNDPROC, (LONG_PTR)proc_entree_commande);
            mettre_a_jour_titre(hwnd);
            charger_fichiers_recents();
            reconstruire_menu_recents();
            lancer_verification_maj(TRUE);
            return 0;

        case WM_SIZE:
            redimensionner_controles(hwnd);
            return 0;

        case WM_COMMAND:
            gerer_commande(hwnd, wp, lp);
            return 0;

        case WM_APP_SORTIE_TEXTE: {
            wchar_t *texte = (wchar_t *)lp;
            ajouter_texte_sortie(texte);
            free(texte);
            return 0;
        }

        case WM_APP_PROCESSUS_TERMINE:
            g_processus_actif = FALSE;
            EnableWindow(g_entree_commande, FALSE);
            if (g_pipe_entree_ecriture) { CloseHandle(g_pipe_entree_ecriture); g_pipe_entree_ecriture = NULL; }
            if (g_processus_courant) g_processus_courant = NULL;
            definir_statut(wp == 0 ? L"Termine (succes)" : L"Termine (code erreur)");
            return 0;

        case WM_APP_MAJ_DISPONIBLE: {
            wchar_t *tag = (wchar_t *)lp;
            wchar_t message[300];
            _snwprintf(message, 300, L"Une nouvelle version est disponible : %s\n\nOuvrir la page de telechargement ?", tag);
            if (MessageBoxW(hwnd, message, L"Mise a jour disponible", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                ShellExecuteW(NULL, L"open", L"https://github.com/abiyeenzo/easy-language/releases/latest", NULL, NULL, SW_SHOWNORMAL);
            }
            free(tag);
            return 0;
        }

        case WM_APP_MAJ_AUCUNE:
            MessageBoxW(hwnd, L"Vous utilisez deja la derniere version.", L"Mise a jour", MB_OK | MB_ICONINFORMATION);
            return 0;

        case WM_APP_MAJ_ERREUR:
            MessageBoxW(hwnd, L"Impossible de verifier les mises a jour (pas de connexion ?).", L"Mise a jour", MB_OK | MB_ICONWARNING);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            if (msg == g_msg_trouver_prochain && g_msg_trouver_prochain != 0) {
                LPFINDREPLACEW fr = (LPFINDREPLACEW)lp;
                if (fr->Flags & FR_DIALOGTERM) {
                    g_dlg_recherche = NULL;
                } else if (fr->Flags & FR_FINDNEXT) {
                    rechercher_suivant(fr->lpstrFindWhat);
                }
                return 0;
            }
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE instance_precedente, PWSTR ligne_commande, int mode_affichage) {
    (void)instance_precedente; (void)ligne_commande;

    LoadLibraryW(L"Msftedit.dll");

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    g_msg_trouver_prochain = RegisterWindowMessageW(FINDMSGSTRINGW);

    HICON icone = LoadIconW(instance, MAKEINTRESOURCEW(IDI_ICONE));

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = FenetrePrincipaleProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"EasyLanguageEditeur";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = icone;
    RegisterClassW(&wc);

    g_fenetre = CreateWindowExW(0, L"EasyLanguageEditeur", L"Easy Language",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 950, 700,
        NULL, NULL, instance, NULL);

    if (icone) {
        SendMessageW(g_fenetre, WM_SETICON, ICON_BIG, (LPARAM)icone);
        SendMessageW(g_fenetre, WM_SETICON, ICON_SMALL, (LPARAM)icone);
    }

    ShowWindow(g_fenetre, mode_affichage);
    UpdateWindow(g_fenetre);

    HACCEL table_accel = CreateAcceleratorTableW((ACCEL[]){
        { FVIRTKEY, VK_F5, ID_MENU_LANCER },
        { FVIRTKEY, VK_F6, ID_MENU_DEBOGUER },
        { FVIRTKEY | FCONTROL, 'N', ID_MENU_NOUVEAU },
        { FVIRTKEY | FCONTROL, 'O', ID_MENU_OUVRIR },
        { FVIRTKEY | FCONTROL, 'S', ID_MENU_ENREGISTRER },
        { FVIRTKEY | FCONTROL, 'F', ID_MENU_RECHERCHER },
    }, 6);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (g_dlg_recherche && IsDialogMessageW(g_dlg_recherche, &msg)) continue;
        if (!TranslateAcceleratorW(g_fenetre, table_accel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return 0;
}
