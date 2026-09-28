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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <winhttp.h>

#include "ressources.h"

#define VERSION_EDITEUR "1.0.9"
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
#define ID_MENU_FERMER_ONGLET 211
#define ID_MENU_VOIR_JOURNAL 212
#define MAX_FICHIERS_RECENTS 8

#define WM_APP_SORTIE_TEXTE (WM_APP + 1)
#define WM_APP_PROCESSUS_TERMINE (WM_APP + 2)
#define WM_APP_MAJ_DISPONIBLE (WM_APP + 3)
#define WM_APP_MAJ_AUCUNE (WM_APP + 4)
#define WM_APP_MAJ_ERREUR (WM_APP + 5)
#define WM_APP_RAFRAICHIR_ARBO (WM_APP + 6)

static HWND g_fenetre, g_editeur, g_sortie, g_entree_commande, g_label_entree;
static HWND g_barre_etat;
static HWND g_gutter;
static WNDPROC g_proc_originale_editeur;
#define LARGEUR_GUTTER 46
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
/* documents ouverts (onglets)                                        */

#define MAX_DOCUMENTS 20
#define LARGEUR_SIDEBAR 220

typedef struct {
    wchar_t chemin[MAX_PATH]; /* vide si "sans titre" (jamais enregistre) */
    wchar_t *contenu;         /* texte hors ecran ; NULL/vide pour l'onglet actif, dont le texte vit dans g_editeur */
    BOOL modifie;
} Document;

static HWND g_onglets;
static Document g_documents[MAX_DOCUMENTS];
static int g_nb_documents = 0;
static int g_document_actif = -1;
static BOOL g_chargement_document = FALSE;

/* ------------------------------------------------------------------ */
/* arborescence (barre laterale)                                      */

#define MAX_ARBO_ENTREES 512

static HWND g_arborescence;
static wchar_t g_arbo_dossier_base[MAX_PATH] = L"";
static wchar_t g_arbo_dossier_en_attente[MAX_PATH] = L"";
static wchar_t g_arbo_noms[MAX_ARBO_ENTREES][MAX_PATH];
static BOOL g_arbo_est_dossier[MAX_ARBO_ENTREES];
static int g_arbo_compte = 0;

static void mettre_a_jour_titre(HWND hwnd);
static BOOL charger_fichier(const wchar_t *chemin);
static void definir_statut(const wchar_t *texte);

/* ------------------------------------------------------------------ */
/* journal (logs) et rapport de plantage                              */
/*                                                                     */
/* L'editeur n'a pas de console visible : sans ceci, une erreur ou un  */
/* plantage disparaissait silencieusement (au mieux une boite de       */
/* dialogue transitoire, jamais gardee). Tout est ecrit, horodate,     */
/* dans %APPDATA%\EasyLanguage\editeur.log, consultable depuis le menu */
/* Aide > Voir le journal. */

static wchar_t g_chemin_journal[MAX_PATH] = L"";

static void initialiser_journal(void) {
    wchar_t appdata[MAX_PATH];
    DWORD longueur = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
    if (longueur == 0 || longueur >= MAX_PATH) return;

    wchar_t dossier[MAX_PATH];
    _snwprintf(dossier, MAX_PATH, L"%s\\EasyLanguage", appdata);
    CreateDirectoryW(dossier, NULL);
    _snwprintf(g_chemin_journal, MAX_PATH, L"%s\\editeur.log", dossier);

    /* evite une croissance illimitee : on repart de zero au-dela de 2 Mo */
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExW(g_chemin_journal, GetFileExInfoStandard, &info)) {
        ULONGLONG taille = ((ULONGLONG)info.nFileSizeHigh << 32) | info.nFileSizeLow;
        if (taille > 2 * 1024 * 1024) DeleteFileW(g_chemin_journal);
    }
}

static void journaliser(const wchar_t *format, ...) {
    if (!g_chemin_journal[0]) return;
    FILE *f = _wfopen(g_chemin_journal, L"a, ccs=UTF-8");
    if (!f) return;

    SYSTEMTIME t;
    GetLocalTime(&t);
    fwprintf(f, L"[%04d-%02d-%02d %02d:%02d:%02d] ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

    va_list args;
    va_start(args, format);
    vfwprintf(f, format, args);
    va_end(args);

    fwprintf(f, L"\n");
    fclose(f);
}

static void ouvrir_journal(void) {
    if (!g_chemin_journal[0]) {
        MessageBoxW(NULL, L"Journal indisponible.", L"Easy Language", MB_ICONWARNING);
        return;
    }
    journaliser(L"Ouverture manuelle du journal par l'utilisateur.");
    ShellExecuteW(NULL, L"open", g_chemin_journal, NULL, NULL, SW_SHOWNORMAL);
}

/* Filtre d'exception non geree (SEH) : capture les plantages reels
   (acces memoire invalide, etc.) qui echapperaient sinon completement,
   et les consigne avant de laisser Windows terminer le processus. Pas
   de trace d'appel symbolisee (demanderait dbghelp.dll et la resolution
   de symboles, hors de portee ici) : le code et l'adresse de
   l'exception suffisent deja a orienter le diagnostic, combines au
   dernier evenement journalise juste avant (quelle action etait en
   cours). */
static volatile LONG g_plantage_en_cours = 0;

static LONG WINAPI filtre_exception_non_geree(EXCEPTION_POINTERS *info) {
    /* Garde de reentrance : si un deuxieme plantage survient pendant le
       traitement du premier (par exemple parce que la boite de dialogue
       ci-dessous force un nouveau rendu d'une fenetre deja corrompue),
       on ne rejoue pas tout le traitement (deuxieme entree de journal,
       deuxieme boite de dialogue qui pompe encore des messages...) :
       terminaison immediate. */
    if (InterlockedCompareExchange(&g_plantage_en_cours, 1, 0) != 0) {
        TerminateProcess(GetCurrentProcess(), 1);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void *adresse = info->ExceptionRecord->ExceptionAddress;
    HMODULE module = NULL;
    /* Identifie le module (exe ou dll) contenant l'adresse fautive :
       "easy_editeur.exe+0x1234" pointe vers notre propre code (utile
       avec le binaire correspondant et un outil comme addr2line, le
       decalage restant stable d'une execution a l'autre malgre l'ASLR
       qui randomise l'adresse absolue) ; un nom de DLL systeme
       (comctl32.dll, user32.dll...) oriente plutot vers un appel Win32
       mal utilise cote appelant plutot qu'un bug direct dans ce fichier. */
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)adresse, &module);
    if (module) {
        wchar_t chemin_module[MAX_PATH] = L"?";
        GetModuleFileNameW(module, chemin_module, MAX_PATH);
        const wchar_t *nom_module = wcsrchr(chemin_module, L'\\');
        nom_module = nom_module ? nom_module + 1 : chemin_module;
        unsigned long long decalage = (unsigned long long)((char *)adresse - (char *)module);
        journaliser(L"PLANTAGE : code 0x%08lX dans %s+0x%llX (adresse absolue 0x%p)",
            (unsigned long)info->ExceptionRecord->ExceptionCode, nom_module, decalage, adresse);
    } else {
        journaliser(L"PLANTAGE : code 0x%08lX a l'adresse 0x%p (module inconnu)",
            (unsigned long)info->ExceptionRecord->ExceptionCode, adresse);
    }

    wchar_t message[300];
    _snwprintf(message, 300,
        L"Easy Language a rencontre un probleme et doit se fermer.\n\nDetails enregistres dans :\n%s",
        g_chemin_journal);
    MessageBoxW(NULL, message, L"Easy Language - Erreur", MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

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
/* numeros de ligne (gouttiere)                                       */

static LRESULT CALLBACK proc_gouttiere(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_ERASEBKGND) return 1;

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH fond = CreateSolidBrush(RGB(238, 236, 242));
        FillRect(dc, &rc, fond);
        DeleteObject(fond);

        /* g_gutter est cree avant g_editeur dans creer_controles (voir
           son commentaire) : avec WS_VISIBLE, un tout premier WM_PAINT
           peut theoriquement arriver avant que g_editeur existe. Garde
           defensive, peu couteuse, pour ne jamais utiliser un handle
           invalide dans les appels qui suivent. */
        if (!g_editeur) { EndPaint(hwnd, &ps); return 0; }

        HFONT police = (HFONT)SendMessageW(g_editeur, WM_GETFONT, 0, 0);
        HFONT ancienne_police = (HFONT)SelectObject(dc, police);
        TEXTMETRICW tm;
        GetTextMetricsW(dc, &tm);
        int hauteur_ligne = tm.tmHeight + tm.tmExternalLeading;

        int premiere_ligne = (int)SendMessageW(g_editeur, EM_GETFIRSTVISIBLELINE, 0, 0);
        int nb_lignes = (int)SendMessageW(g_editeur, EM_GETLINECOUNT, 0, 0);

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(142, 138, 156));

        int y = 0;
        for (int ligne = premiere_ligne; ligne < nb_lignes && y < rc.bottom; ligne++) {
            wchar_t texte[16];
            _snwprintf(texte, 16, L"%d", ligne + 1);
            RECT zone = { 0, y, rc.right - 8, y + hauteur_ligne };
            DrawTextW(dc, texte, -1, &zone, DT_RIGHT | DT_SINGLELINE | DT_NOCLIP);
            y += hauteur_ligne;
        }

        SelectObject(dc, ancienne_police);
        EndPaint(hwnd, &ps);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void invalider_gouttiere(void) {
    if (g_gutter) InvalidateRect(g_gutter, NULL, TRUE);
}

/* Sous-classe l'editeur pour repeindre la gouttiere apres tout ce qui
   peut faire defiler ou re-paginer le texte (RichEdit n'envoie pas de
   notification dediee pour le simple defilement). */
static LRESULT CALLBACK proc_editeur(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    LRESULT resultat = CallWindowProcW(g_proc_originale_editeur, hwnd, msg, wp, lp);
    switch (msg) {
        case WM_VSCROLL:
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN:
        case WM_SIZE:
            invalider_gouttiere();
            break;
    }
    return resultat;
}

/* ------------------------------------------------------------------ */
/* documents ouverts (onglets)                                        */

static void mettre_a_jour_etiquette_onglet(int index) {
    const wchar_t *chemin = g_documents[index].chemin;
    const wchar_t *nom = chemin[0] ? wcsrchr(chemin, L'\\') : NULL;
    nom = nom ? nom + 1 : (chemin[0] ? chemin : L"Sans titre");
    wchar_t etiquette[MAX_PATH + 4];
    _snwprintf(etiquette, MAX_PATH + 4, L"%s%s", nom, g_documents[index].modifie ? L" *" : L"");

    TCITEMW item;
    memset(&item, 0, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = etiquette;
    SendMessageW(g_onglets, TCM_SETITEMW, (WPARAM)index, (LPARAM)&item);
}

/* Sauvegarde le texte actuellement affiche dans g_editeur vers le
   document actif (hors ecran), avant de basculer vers un autre onglet :
   un seul RichEdit est partage entre tous les onglets plutot que d'en
   garder N en memoire, le contenu "non actif" vit dans Document.contenu. */
static void capturer_document_actif(void) {
    if (g_document_actif < 0) return;
    int longueur = GetWindowTextLengthW(g_editeur);
    wchar_t *texte = malloc(sizeof(wchar_t) * (longueur + 1));
    GetWindowTextW(g_editeur, texte, longueur + 1);
    free(g_documents[g_document_actif].contenu);
    g_documents[g_document_actif].contenu = texte;
}

static void afficher_document(int index) {
    if (index < 0 || index >= g_nb_documents) return;
    g_document_actif = index;

    g_chargement_document = TRUE;
    SetWindowTextW(g_editeur, g_documents[index].contenu ? g_documents[index].contenu : L"");
    g_chargement_document = FALSE;

    wcsncpy(g_chemin_fichier, g_documents[index].chemin, MAX_PATH - 1);
    g_chemin_fichier[MAX_PATH - 1] = L'\0';

    colorer_syntaxe(g_editeur);
    invalider_gouttiere();
    mettre_a_jour_titre(g_fenetre);
    SendMessageW(g_onglets, TCM_SETCURSEL, (WPARAM)index, 0);
}

static void nouveau_document(void) {
    if (g_nb_documents >= MAX_DOCUMENTS) {
        MessageBoxW(g_fenetre, L"Nombre maximal de fichiers ouverts atteint.", L"Easy Language", MB_ICONWARNING);
        return;
    }
    capturer_document_actif();

    int index = g_nb_documents++;
    g_documents[index].chemin[0] = L'\0';
    g_documents[index].contenu = _wcsdup(L"");
    g_documents[index].modifie = FALSE;

    TCITEMW item;
    memset(&item, 0, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = L"Sans titre";
    SendMessageW(g_onglets, TCM_INSERTITEMW, (WPARAM)index, (LPARAM)&item);

    afficher_document(index);
    definir_statut(L"Pret");
}

/* Ferme l'onglet actif (Ctrl+W). Garde toujours au moins un onglet
   ouvert : fermer le dernier le remplace par un nouvel onglet vide
   plutot que de laisser l'editeur sans onglet du tout. */
static void fermer_onglet_actif(void) {
    if (g_document_actif < 0) return;

    if (g_documents[g_document_actif].modifie) {
        int reponse = MessageBoxW(g_fenetre,
            L"Ce fichier contient des modifications non enregistrees. Fermer quand meme ?",
            L"Fermer l'onglet", MB_YESNO | MB_ICONWARNING);
        if (reponse != IDYES) return;
    }

    free(g_documents[g_document_actif].contenu);
    int ferme = g_document_actif;
    for (int i = ferme; i < g_nb_documents - 1; i++) {
        g_documents[i] = g_documents[i + 1];
    }
    g_nb_documents--;
    g_document_actif = -1;
    SendMessageW(g_onglets, TCM_DELETEITEM, (WPARAM)ferme, 0);

    if (g_nb_documents == 0) {
        nouveau_document();
    } else {
        int nouvel_index = ferme < g_nb_documents ? ferme : g_nb_documents - 1;
        afficher_document(nouvel_index);
    }
}

/* ------------------------------------------------------------------ */
/* arborescence (barre laterale)                                      */

static void obtenir_dossier_parent(const wchar_t *dossier, wchar_t *dehors, size_t taille) {
    wcsncpy(dehors, dossier, taille - 1);
    dehors[taille - 1] = L'\0';
    wchar_t *sep = wcsrchr(dehors, L'\\');
    if (sep && sep != dehors) *sep = L'\0';
}

/* Rafraichir l'arborescence (TreeView_DeleteAllItems + repeuplement)
   DEPUIS le gestionnaire de sa propre notification TVN_SELCHANGEDW est
   un plantage classique en Win32 : on detruirait les elements du
   controle alors que comctl32 est encore en train de terminer son
   propre traitement du clic, plus bas sur la meme pile d'appel (une
   notification WM_NOTIFY est livree de facon synchrone, comme un appel
   de fonction imbrique). D'ou la reconstruction differee : on poste un
   message a soi-meme, traite seulement une fois revenu a la boucle de
   messages, apres que la notification d'origine ait fini de se
   derouler. Utilisee pour tout rafraichissement declenche par un clic
   dans l'arborescence elle-meme (fichier ouvert, dossier navigue) ; le
   peuplement initial au demarrage (avant tout clic) reste direct, sans
   risque de reentrance. */
static void demander_rafraichissement_arborescence(const wchar_t *dossier) {
    wcsncpy(g_arbo_dossier_en_attente, dossier, MAX_PATH - 1);
    g_arbo_dossier_en_attente[MAX_PATH - 1] = L'\0';
    PostMessageW(g_fenetre, WM_APP_RAFRAICHIR_ARBO, 0, 0);
}

static void peupler_arborescence(const wchar_t *dossier) {
    TreeView_DeleteAllItems(g_arborescence);
    g_arbo_compte = 0;
    if (!dossier || !dossier[0]) return;
    wcsncpy(g_arbo_dossier_base, dossier, MAX_PATH - 1);
    g_arbo_dossier_base[MAX_PATH - 1] = L'\0';

    TVINSERTSTRUCTW is;
    memset(&is, 0, sizeof(is));
    is.hParent = TVI_ROOT;
    is.hInsertAfter = TVI_LAST;
    is.item.mask = TVIF_TEXT | TVIF_PARAM;
    is.item.pszText = L"..";
    is.item.lParam = -1;
    TreeView_InsertItem(g_arborescence, &is);

    wchar_t motif[MAX_PATH];
    _snwprintf(motif, MAX_PATH, L"%s\\*", dossier);
    WIN32_FIND_DATAW donnee;
    HANDLE h = FindFirstFileW(motif, &donnee);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(donnee.cFileName, L".") == 0 || wcscmp(donnee.cFileName, L"..") == 0) continue;
        BOOL est_dossier = (donnee.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        size_t longueur = wcslen(donnee.cFileName);
        BOOL fichier_elg = !est_dossier && longueur > 4 && _wcsicmp(donnee.cFileName + longueur - 4, L".elg") == 0;
        if (!est_dossier && !fichier_elg) continue;
        if (g_arbo_compte >= MAX_ARBO_ENTREES) break;

        int idx = g_arbo_compte++;
        wcsncpy(g_arbo_noms[idx], donnee.cFileName, MAX_PATH - 1);
        g_arbo_noms[idx][MAX_PATH - 1] = L'\0';
        g_arbo_est_dossier[idx] = est_dossier;

        wchar_t etiquette[MAX_PATH + 4];
        _snwprintf(etiquette, MAX_PATH + 4, est_dossier ? L"[%s]" : L"%s", donnee.cFileName);
        is.item.pszText = etiquette;
        is.item.lParam = (LPARAM)idx;
        TreeView_InsertItem(g_arborescence, &is);
    } while (FindNextFileW(h, &donnee));
    FindClose(h);
}

static void gerer_clic_arborescence(int idx) {
    if (idx == -1) {
        wchar_t parent[MAX_PATH];
        obtenir_dossier_parent(g_arbo_dossier_base, parent, MAX_PATH);
        demander_rafraichissement_arborescence(parent);
        return;
    }
    if (idx < 0 || idx >= g_arbo_compte) return;

    wchar_t chemin[MAX_PATH];
    _snwprintf(chemin, MAX_PATH, L"%s\\%s", g_arbo_dossier_base, g_arbo_noms[idx]);
    if (g_arbo_est_dossier[idx]) {
        demander_rafraichissement_arborescence(chemin);
    } else if (!charger_fichier(chemin)) {
        journaliser(L"Echec d'ouverture depuis l'arborescence : %s", chemin);
        MessageBoxW(g_fenetre, L"Impossible d'ouvrir ce fichier.", L"Erreur", MB_ICONERROR);
    }
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
    journaliser(L"Lancement demande : %s (fichier %s)", nom_exe, g_chemin_fichier);
    if (g_processus_actif) {
        MessageBoxW(g_fenetre, L"Une execution est deja en cours.", L"Easy Language", MB_ICONINFORMATION);
        return;
    }

    wchar_t chemin_exe[MAX_PATH];
    if (!obtenir_chemin_frere(nom_exe, chemin_exe, MAX_PATH)) {
        journaliser(L"Executable introuvable a cote de l'editeur : %s", nom_exe);
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
        journaliser(L"CreateProcessW a echoue (code %lu) pour : %s", (unsigned long)GetLastError(), ligne_commande);
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
static void obtenir_dossier_w(const wchar_t *chemin, wchar_t *dehors, size_t taille) {
    wcsncpy(dehors, chemin, taille - 1);
    dehors[taille - 1] = L'\0';
    wchar_t *sep = wcsrchr(dehors, L'\\');
    if (sep) *sep = L'\0';
}

static BOOL charger_fichier(const wchar_t *chemin) {
    /* deja ouvert dans un onglet ? on y bascule plutot que d'ouvrir un doublon */
    for (int i = 0; i < g_nb_documents; i++) {
        if (_wcsicmp(g_documents[i].chemin, chemin) == 0) {
            capturer_document_actif();
            afficher_document(i);
            ajouter_fichier_recent(chemin);
            return TRUE;
        }
    }

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
    free(utf8);

    if (g_nb_documents >= MAX_DOCUMENTS) {
        MessageBoxW(g_fenetre, L"Nombre maximal de fichiers ouverts atteint.", L"Easy Language", MB_ICONWARNING);
        free(large);
        return FALSE;
    }

    capturer_document_actif();

    /* reutilise l'onglet actif s'il s'agit d'un "Sans titre" vide et non
       modifie, plutot que de laisser un onglet inutile trainer */
    int index;
    BOOL onglet_actif_vide = g_document_actif >= 0 && g_documents[g_document_actif].chemin[0] == L'\0' &&
        !g_documents[g_document_actif].modifie &&
        (!g_documents[g_document_actif].contenu || g_documents[g_document_actif].contenu[0] == L'\0');
    if (onglet_actif_vide) {
        index = g_document_actif;
    } else {
        index = g_nb_documents++;
        TCITEMW item;
        memset(&item, 0, sizeof(item));
        item.mask = TCIF_TEXT;
        item.pszText = L"";
        SendMessageW(g_onglets, TCM_INSERTITEMW, (WPARAM)index, (LPARAM)&item);
    }

    wcsncpy(g_documents[index].chemin, chemin, MAX_PATH - 1);
    g_documents[index].chemin[MAX_PATH - 1] = L'\0';
    free(g_documents[index].contenu);
    g_documents[index].contenu = large;
    g_documents[index].modifie = FALSE;

    afficher_document(index);
    mettre_a_jour_etiquette_onglet(index);
    ajouter_fichier_recent(chemin);

    wchar_t dossier[MAX_PATH];
    obtenir_dossier_w(chemin, dossier, MAX_PATH);
    demander_rafraichissement_arborescence(dossier);
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
    BOOL ok = f != INVALID_HANDLE_VALUE;
    DWORD ecrit = 0;
    if (ok) {
        WriteFile(f, utf8, (DWORD)strlen(utf8), &ecrit, NULL);
        ok = ecrit == strlen(utf8);
        CloseHandle(f);
    }
    free(texte);
    free(utf8);

    /* echec reel d'ecriture (pas le cas "pas encore de chemin", deja
       ecarte au tout debut de la fonction) : auparavant signale comme un
       succes silencieux, l'utilisateur croyait avoir enregistre alors
       que non. */
    if (!ok) {
        journaliser(L"Echec d'enregistrement (GetLastError=%lu) : %s", (unsigned long)GetLastError(), g_chemin_fichier);
        wchar_t message[MAX_PATH + 64];
        _snwprintf(message, MAX_PATH + 64, L"Impossible d'enregistrer le fichier :\n%s", g_chemin_fichier);
        MessageBoxW(g_fenetre, message, L"Erreur", MB_ICONERROR);
    }

    if (ok && g_document_actif >= 0) {
        wcsncpy(g_documents[g_document_actif].chemin, g_chemin_fichier, MAX_PATH - 1);
        g_documents[g_document_actif].chemin[MAX_PATH - 1] = L'\0';
        g_documents[g_document_actif].modifie = FALSE;
        mettre_a_jour_etiquette_onglet(g_document_actif);
    }
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
        journaliser(L"Echec d'ouverture via la boite de dialogue : %s", tampon);
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
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_FERMER_ONGLET, L"Fermer l'onglet\tCtrl+W");
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
    AppendMenuW(menu_aide, MF_STRING, ID_MENU_VOIR_JOURNAL, L"Voir le journal (logs)");
    AppendMenuW(menu_aide, MF_STRING, ID_MENU_APROPOS, L"A propos");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_aide, L"Aide");

    SetMenu(hwnd, barre);
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

    creer_barre_etat(hwnd);

    g_onglets = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, instance, NULL);
    SendMessageW(g_onglets, WM_SETFONT, (WPARAM)police, TRUE);

    g_arborescence = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_SHOWSELALWAYS,
        0, 0, 0, 0, hwnd, NULL, instance, NULL);
    SendMessageW(g_arborescence, WM_SETFONT, (WPARAM)police, TRUE);

    g_gutter = CreateWindowExW(0, L"EasyLanguageGouttiere", L"",
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, instance, NULL);

    /* WS_HSCROLL | ES_AUTOHSCROLL desactive le retour a la ligne
       automatique : une ligne logique = une ligne visuelle a l'ecran,
       necessaire pour que les numeros de la gouttiere restent alignes
       avec le texte (retour a la ligne + gouttiere ne feraient plus
       correspondre les deux sans un calcul de pagination bien plus
       complexe). Une barre de defilement horizontale apparait quand une
       ligne depasse la largeur visible. */
    g_editeur = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
        0, 0, 0, 0, hwnd, (HMENU)ID_EDITEUR, instance, NULL);
    SendMessageW(g_editeur, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
    SendMessageW(g_editeur, WM_SETFONT, (WPARAM)police, TRUE);
    g_proc_originale_editeur = (WNDPROC)SetWindowLongPtrW(g_editeur, GWLP_WNDPROC, (LONG_PTR)proc_editeur);

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
    int largeur_totale = rc.right - rc.left;
    int hauteur = rc.bottom - rc.top;

    SendMessageW(g_barre_etat, WM_SIZE, 0, 0);
    RECT rc_etat;
    GetWindowRect(g_barre_etat, &rc_etat);
    int h_etat = rc_etat.bottom - rc_etat.top;

    int h_onglets = 28;
    MoveWindow(g_onglets, 0, 0, largeur_totale, h_onglets, TRUE);

    int y = h_onglets;
    int zone_utile = hauteur - h_onglets - h_etat;

    MoveWindow(g_arborescence, 0, y, LARGEUR_SIDEBAR, zone_utile, TRUE);

    int x = LARGEUR_SIDEBAR;
    int largeur = largeur_totale - LARGEUR_SIDEBAR;

    int h_editeur = (int)(zone_utile * 0.6);
    MoveWindow(g_gutter, x, y, LARGEUR_GUTTER, h_editeur, TRUE);
    MoveWindow(g_editeur, x + LARGEUR_GUTTER, y, largeur - LARGEUR_GUTTER, h_editeur, TRUE);
    y += h_editeur;

    int h_sortie = zone_utile - h_editeur - 44;
    if (h_sortie < 50) h_sortie = 50;
    MoveWindow(g_sortie, x, y, largeur, h_sortie, TRUE);
    y += h_sortie;

    MoveWindow(g_label_entree, x, y, largeur, 18, TRUE);
    y += 18;
    MoveWindow(g_entree_commande, x, y, largeur, 24, TRUE);
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
        invalider_gouttiere();
        if (!g_chargement_document && g_document_actif >= 0 && !g_documents[g_document_actif].modifie) {
            g_documents[g_document_actif].modifie = TRUE;
            mettre_a_jour_etiquette_onglet(g_document_actif);
        }
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
            nouveau_document();
            break;
        case ID_MENU_FERMER_ONGLET:
            fermer_onglet_actif();
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
        case ID_MENU_VOIR_JOURNAL:
            ouvrir_journal();
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
        case WM_CREATE: {
            /* g_fenetre n'est normalement assigne qu'au retour de
               CreateWindowExW, mais WM_CREATE est distribue de maniere
               synchrone pendant cet appel : les fonctions appelees d'ici
               (nouveau_document -> afficher_document -> mettre_a_jour_titre)
               utilisent g_fenetre, il faut donc l'avoir deja assigne. */
            g_fenetre = hwnd;
            creer_menu(hwnd);
            creer_controles(hwnd);
            g_proc_originale_commande = (WNDPROC)SetWindowLongPtrW(g_entree_commande, GWLP_WNDPROC, (LONG_PTR)proc_entree_commande);
            nouveau_document();
            mettre_a_jour_titre(hwnd);
            charger_fichiers_recents();
            reconstruire_menu_recents();
            /* Le dossier courant (CWD) d'une appli GUI lancee par raccourci
               n'est pas previsible (peut etre le Bureau, System32, ou
               n'importe quoi selon comment elle a ete lancee) : la barre
               laterale semblait alors vide ou montrait un dossier sans
               rapport, un peu comme VS Code sans dossier ouvert. Le
               dossier de l'executable est stable et toujours pertinent
               (il contient bibliotheque/ et exemples/), donc c'est le
               point de depart par defaut. */
            wchar_t dossier_defaut[MAX_PATH];
            if (obtenir_chemin_frere(L"", dossier_defaut, MAX_PATH)) {
                size_t longueur = wcslen(dossier_defaut);
                if (longueur > 0 && dossier_defaut[longueur - 1] == L'\\') dossier_defaut[longueur - 1] = L'\0';
                peupler_arborescence(dossier_defaut);
            }
            lancer_verification_maj(TRUE);
            return 0;
        }

        case WM_SIZE:
            redimensionner_controles(hwnd);
            return 0;

        case WM_COMMAND:
            gerer_commande(hwnd, wp, lp);
            return 0;

        case WM_NOTIFY: {
            NMHDR *nm = (NMHDR *)lp;
            if (nm->hwndFrom == g_onglets && nm->code == TCN_SELCHANGE) {
                capturer_document_actif();
                int nouvel_index = (int)SendMessageW(g_onglets, TCM_GETCURSEL, 0, 0);
                afficher_document(nouvel_index);
            } else if (nm->hwndFrom == g_arborescence && nm->code == TVN_SELCHANGEDW) {
                NMTREEVIEWW *nmtv = (NMTREEVIEWW *)lp;
                gerer_clic_arborescence((int)nmtv->itemNew.lParam);
            }
            return 0;
        }

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
            journaliser(L"Execution terminee, code de sortie %lu", (unsigned long)wp);
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

        case WM_APP_RAFRAICHIR_ARBO:
            peupler_arborescence(g_arbo_dossier_en_attente);
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

    initialiser_journal();
    SetUnhandledExceptionFilter(filtre_exception_non_geree);
    journaliser(L"Demarrage Easy Language " VERSION_EDITEUR_L L".");

    LoadLibraryW(L"Msftedit.dll");

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES;
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

    WNDCLASSW wc_gouttiere;
    memset(&wc_gouttiere, 0, sizeof(wc_gouttiere));
    wc_gouttiere.lpfnWndProc = proc_gouttiere;
    wc_gouttiere.hInstance = instance;
    wc_gouttiere.lpszClassName = L"EasyLanguageGouttiere";
    wc_gouttiere.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassW(&wc_gouttiere);

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
        { FVIRTKEY | FCONTROL, 'W', ID_MENU_FERMER_ONGLET },
    }, 7);

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
