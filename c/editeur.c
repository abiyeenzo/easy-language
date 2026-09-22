/* Editeur graphique Easy Language, en Win32 pur (aucune dependance externe
   hors de l'API Windows standard: user32, gdi32, comdlg32, comctl32, et
   Msftedit.dll pour le controle RichEdit fourni avec Windows).

   Fonctionnalites:
   - Edition avec coloration syntaxique basique (mots-cles, chaines,
     nombres, commentaires) via RichEdit + EM_SETCHARFORMAT.
   - Ouvrir / Enregistrer / Enregistrer sous.
   - F5 Lancer: sauvegarde le fichier, lance easy_language.exe (suppose
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

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

#define WM_APP_SORTIE_TEXTE (WM_APP + 1)
#define WM_APP_PROCESSUS_TERMINE (WM_APP + 2)

static HWND g_fenetre, g_editeur, g_sortie, g_entree_commande, g_label_entree;
static wchar_t g_chemin_fichier[MAX_PATH] = L"";
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
    CloseHandle(p->processus);
    PostMessageW(g_fenetre, WM_APP_PROCESSUS_TERMINE, 0, 0);
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
        CloseHandle(sortie_lecture);
        CloseHandle(entree_ecriture);
        return;
    }

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
    return enregistrer_fichier();
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

    HANDLE f = CreateFileW(tampon, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
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

    wcsncpy(g_chemin_fichier, tampon, MAX_PATH);
    colorer_syntaxe(g_editeur);
}

/* ------------------------------------------------------------------ */
/* interface                                                          */

static void creer_menu(HWND hwnd) {
    HMENU barre = CreateMenu();

    HMENU menu_fichier = CreatePopupMenu();
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_NOUVEAU, L"Nouveau\tCtrl+N");
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_OUVRIR, L"Ouvrir...\tCtrl+O");
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_ENREGISTRER, L"Enregistrer\tCtrl+S");
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_ENREGISTRER_SOUS, L"Enregistrer sous...");
    AppendMenuW(menu_fichier, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu_fichier, MF_STRING, ID_MENU_QUITTER, L"Quitter");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_fichier, L"Fichier");

    HMENU menu_executer = CreatePopupMenu();
    AppendMenuW(menu_executer, MF_STRING, ID_MENU_LANCER, L"Lancer\tF5");
    AppendMenuW(menu_executer, MF_STRING, ID_MENU_DEBOGUER, L"Deboguer\tF6");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_executer, L"Executer");

    HMENU menu_aide = CreatePopupMenu();
    AppendMenuW(menu_aide, MF_STRING, ID_MENU_APROPOS, L"A propos");
    AppendMenuW(barre, MF_POPUP, (UINT_PTR)menu_aide, L"Aide");

    SetMenu(hwnd, barre);
}

static void creer_controles(HWND hwnd) {
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    HFONT police = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                FIXED_PITCH, L"Consolas");

    g_editeur = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        0, 0, 0, 0, hwnd, (HMENU)ID_EDITEUR, instance, NULL);
    SendMessageW(g_editeur, EM_SETEVENTMASK, 0, ENM_CHANGE);
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

    int y = 0;
    int h_editeur = (int)(hauteur * 0.6);
    MoveWindow(g_editeur, 0, y, largeur, h_editeur, TRUE);
    y += h_editeur;

    int h_sortie = hauteur - y - 44;
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
        wcscpy(titre, L"Easy Language 1.0.0 - Editeur [Nouveau fichier]");
    } else {
        _snwprintf(titre, MAX_PATH + 64, L"Easy Language 1.0.0 - Editeur [%s]", g_chemin_fichier);
    }
    SetWindowTextW(hwnd, titre);
}

static void gerer_commande(HWND hwnd, WPARAM wp, LPARAM lp) {
    int id = LOWORD(wp);
    int notification = HIWORD(wp);

    if ((HWND)lp == g_editeur && notification == EN_CHANGE) {
        colorer_syntaxe(g_editeur);
        return;
    }

    switch (id) {
        case ID_MENU_NOUVEAU:
            SetWindowTextW(g_editeur, L"");
            g_chemin_fichier[0] = L'\0';
            mettre_a_jour_titre(hwnd);
            break;
        case ID_MENU_OUVRIR:
            ouvrir_fichier();
            mettre_a_jour_titre(hwnd);
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
            lancer_processus(L"easy_language.exe", NULL, FALSE);
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
                L"Easy Language 1.0.0\n\nLangage de programmation interprete en francais.\nFichiers .elg\n\nhttps://github.com/abiyeenzo/easy-language",
                L"A propos", MB_OK);
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
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE instance_precedente, PWSTR ligne_commande, int mode_affichage) {
    (void)instance_precedente; (void)ligne_commande;

    LoadLibraryW(L"Msftedit.dll");

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = FenetrePrincipaleProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"EasyLanguageEditeur";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);

    g_fenetre = CreateWindowExW(0, L"EasyLanguageEditeur", L"Easy Language",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 950, 700,
        NULL, NULL, instance, NULL);

    ShowWindow(g_fenetre, mode_affichage);
    UpdateWindow(g_fenetre);

    HACCEL table_accel = CreateAcceleratorTableW((ACCEL[]){
        { FVIRTKEY, VK_F5, ID_MENU_LANCER },
        { FVIRTKEY, VK_F6, ID_MENU_DEBOGUER },
        { FVIRTKEY | FCONTROL, 'N', ID_MENU_NOUVEAU },
        { FVIRTKEY | FCONTROL, 'O', ID_MENU_OUVRIR },
        { FVIRTKEY | FCONTROL, 'S', ID_MENU_ENREGISTRER },
    }, 5);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(g_fenetre, table_accel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return 0;
}
