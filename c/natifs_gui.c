#include "natifs_gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreteur.h"

#ifdef _WIN32
/* Ce fichier n'utilise que les API explicitement suffixees "W" (large,
   UTF-16), jamais les macros generiques type TCHAR (IDC_ARROW,
   CreateWindow sans suffixe, ...) : UNICODE/_UNICODE doivent donc etre
   definis avant d'inclure windows.h pour que ces macros generiques (par
   exemple IDC_ARROW, utilise plus bas) se resolvent vers leur variante
   large plutot que l'ANSI par defaut. */
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

static wchar_t *vers_large(const char *texte) {
    int longueur = MultiByteToWideChar(CP_UTF8, 0, texte, -1, NULL, 0);
    wchar_t *large = malloc(sizeof(wchar_t) * longueur);
    MultiByteToWideChar(CP_UTF8, 0, texte, -1, large, longueur);
    return large;
}

static char *vers_utf8(const wchar_t *large) {
    int taille = WideCharToMultiByte(CP_UTF8, 0, large, -1, NULL, 0, NULL, NULL);
    char *utf8 = malloc(taille);
    WideCharToMultiByte(CP_UTF8, 0, large, -1, utf8, taille, NULL, NULL);
    return utf8;
}

/* ------------------------------------------------------------------ */
/* fenetres et composants graphiques reels (fenetre_*)                */
/*                                                                     */
/* Modele simple par identifiants entiers (comme les sockets de       */
/* natifs_reseau.c) : gui.creer()/gui.bouton()/... retournent un       */
/* entier qui indexe une table C, plutot que d'exposer un HWND au      */
/* script. gui.executer() lance la boucle de messages Win32 ; les      */
/* clics sur les boutons enregistres via gui.sur_clic() rappellent     */
/* directement une fonction Easy Language (interpreteur_appeler),      */
/* comme un vrai gestionnaire d'evenements.                            */

#define MAX_FENETRES_GUI 16
#define MAX_WIDGETS_GUI 256

typedef struct {
    HWND hwnd;
    BOOL active;
} FenetreGUI;

typedef struct {
    HWND hwnd;
    Valeur callback;
    BOOL a_callback;
} WidgetGUI;

static FenetreGUI g_fenetres_gui[MAX_FENETRES_GUI];
static int g_nb_fenetres_gui = 0;
static int g_fenetres_actives_gui = 0;

static WidgetGUI g_widgets_gui[MAX_WIDGETS_GUI];
static int g_nb_widgets_gui = 0;

static ATOM g_classe_fenetre_gui = 0;

static LRESULT CALLBACK proc_fenetre_gui(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND) {
        int id = LOWORD(wp);
        int notification = HIWORD(wp);
        if (notification == BN_CLICKED) {
            int idx = id - 2000;
            if (idx >= 0 && idx < g_nb_widgets_gui && g_widgets_gui[idx].a_callback) {
                interpreteur_appeler(g_widgets_gui[idx].callback, NULL, 0, 0);
            }
        }
        return 0;
    }
    if (msg == WM_CLOSE) {
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        for (int i = 0; i < g_nb_fenetres_gui; i++) {
            if (g_fenetres_gui[i].hwnd == hwnd && g_fenetres_gui[i].active) {
                g_fenetres_gui[i].active = FALSE;
                g_fenetres_actives_gui--;
            }
        }
        if (g_fenetres_actives_gui <= 0) PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void assurer_classe_fenetre_gui(void) {
    if (g_classe_fenetre_gui) return;
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = proc_fenetre_gui;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"EasyLanguageGuiFenetre";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    g_classe_fenetre_gui = RegisterClassW(&wc);
}

static HWND obtenir_hwnd_fenetre(Valeur v, int ligne) {
    if (!valeur_est_nombre(v)) interpreteur_erreur(ligne, "identifiant de fenetre invalide");
    int idx = (int)valeur_comme_reel(v);
    if (idx < 0 || idx >= g_nb_fenetres_gui || !g_fenetres_gui[idx].active) {
        interpreteur_erreur(ligne, "fenetre invalide ou deja fermee");
    }
    return g_fenetres_gui[idx].hwnd;
}

static HWND obtenir_hwnd_widget(Valeur v, int ligne) {
    if (!valeur_est_nombre(v)) interpreteur_erreur(ligne, "identifiant de composant invalide");
    int idx = (int)valeur_comme_reel(v);
    if (idx < 0 || idx >= g_nb_widgets_gui) interpreteur_erreur(ligne, "identifiant de composant invalide");
    return g_widgets_gui[idx].hwnd;
}

static int creer_widget(HWND parent, const wchar_t *classe, const wchar_t *texte, DWORD style,
                         int x, int y, int largeur, int hauteur, int ligne) {
    if (g_nb_widgets_gui >= MAX_WIDGETS_GUI) interpreteur_erreur(ligne, "trop de composants graphiques");
    int idx = g_nb_widgets_gui++;
    HWND hwnd = CreateWindowExW(0, classe, texte, WS_CHILD | WS_VISIBLE | style,
        x, y, largeur, hauteur, parent, (HMENU)(INT_PTR)(2000 + idx), GetModuleHandleW(NULL), NULL);
    if (!hwnd) interpreteur_erreur(ligne, "impossible de creer le composant graphique");
    g_widgets_gui[idx].hwnd = hwnd;
    g_widgets_gui[idx].a_callback = FALSE;
    return idx;
}

#endif /* _WIN32 */

int natifs_gui_est(const char *nom) {
    static const char *noms[] = {
        "boite_message", "boite_question",
        "fenetre_creer", "fenetre_bouton", "fenetre_etiquette", "fenetre_champ_texte",
        "fenetre_lire_champ", "fenetre_definir_champ", "fenetre_sur_clic",
        "fenetre_executer", "fenetre_fermer",
    };
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

static void exiger_texte(Valeur v, int ligne, const char *nom_fonction) {
    if (v.type != V_TEXTE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' attend du texte", nom_fonction);
        interpreteur_erreur(ligne, msg);
    }
}

#ifndef _WIN32
static Valeur erreur_windows_uniquement(int ligne) {
    interpreteur_erreur(ligne, "les fenetres graphiques (fenetre_*) ne sont disponibles que sous Windows ; boite_message/boite_question restent utilisables partout");
    return valeur_rien();
}
#endif

Valeur natifs_gui_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    if (strcmp(nom, "boite_message") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'boite_message' attend 2 arguments: titre et texte");
        exiger_texte(args[0], ligne, "boite_message");
        exiger_texte(args[1], ligne, "boite_message");
#ifdef _WIN32
        wchar_t *titre = vers_large(args[0].comme.texte);
        wchar_t *texte = vers_large(args[1].comme.texte);
        MessageBoxW(NULL, texte, titre, MB_OK | MB_ICONINFORMATION);
        free(titre);
        free(texte);
#else
        /* Pas de fenetrage sur cette plateforme (dev/tests Linux) : repli
           console, pour que les scripts utilisant boite_message restent
           executables et testables ici. Sous Windows (.exe), une vraie
           boite de dialogue s'affiche via MessageBoxW. */
        printf("[%s] %s\n", args[0].comme.texte, args[1].comme.texte);
#endif
        return valeur_rien();
    }

    if (strcmp(nom, "boite_question") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'boite_question' attend 2 arguments: titre et texte");
        exiger_texte(args[0], ligne, "boite_question");
        exiger_texte(args[1], ligne, "boite_question");
#ifdef _WIN32
        wchar_t *titre = vers_large(args[0].comme.texte);
        wchar_t *texte = vers_large(args[1].comme.texte);
        int reponse = MessageBoxW(NULL, texte, titre, MB_YESNO | MB_ICONQUESTION);
        free(titre);
        free(texte);
        return valeur_booleen(reponse == IDYES);
#else
        printf("[%s] %s (o/n) ", args[0].comme.texte, args[1].comme.texte);
        fflush(stdout);
        char ligne_lue[16];
        if (!fgets(ligne_lue, sizeof(ligne_lue), stdin)) return valeur_booleen(0);
        return valeur_booleen(ligne_lue[0] == 'o' || ligne_lue[0] == 'O');
#endif
    }

    if (strcmp(nom, "fenetre_creer") == 0) {
        if (nb_args != 3) interpreteur_erreur(ligne, "'fenetre_creer' attend 3 arguments: titre, largeur, hauteur");
        exiger_texte(args[0], ligne, "fenetre_creer");
        if (!valeur_est_nombre(args[1]) || !valeur_est_nombre(args[2])) {
            interpreteur_erreur(ligne, "'fenetre_creer' attend des dimensions numeriques");
        }
#ifdef _WIN32
        assurer_classe_fenetre_gui();
        if (g_nb_fenetres_gui >= MAX_FENETRES_GUI) interpreteur_erreur(ligne, "trop de fenetres ouvertes");
        wchar_t *titre = vers_large(args[0].comme.texte);
        HWND hwnd = CreateWindowExW(0, L"EasyLanguageGuiFenetre", titre, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, (int)valeur_comme_reel(args[1]), (int)valeur_comme_reel(args[2]),
            NULL, NULL, GetModuleHandleW(NULL), NULL);
        free(titre);
        if (!hwnd) interpreteur_erreur(ligne, "impossible de creer la fenetre");
        ShowWindow(hwnd, SW_SHOW);
        int idx = g_nb_fenetres_gui++;
        g_fenetres_gui[idx].hwnd = hwnd;
        g_fenetres_gui[idx].active = TRUE;
        g_fenetres_actives_gui++;
        return valeur_entier(idx);
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_bouton") == 0) {
        if (nb_args != 6) interpreteur_erreur(ligne, "'fenetre_bouton' attend 6 arguments: fenetre, texte, x, y, largeur, hauteur");
        exiger_texte(args[1], ligne, "fenetre_bouton");
#ifdef _WIN32
        HWND parent = obtenir_hwnd_fenetre(args[0], ligne);
        wchar_t *texte = vers_large(args[1].comme.texte);
        int idx = creer_widget(parent, L"BUTTON", texte, BS_PUSHBUTTON,
            (int)valeur_comme_reel(args[2]), (int)valeur_comme_reel(args[3]),
            (int)valeur_comme_reel(args[4]), (int)valeur_comme_reel(args[5]), ligne);
        free(texte);
        return valeur_entier(idx);
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_etiquette") == 0) {
        if (nb_args != 6) interpreteur_erreur(ligne, "'fenetre_etiquette' attend 6 arguments: fenetre, texte, x, y, largeur, hauteur");
        exiger_texte(args[1], ligne, "fenetre_etiquette");
#ifdef _WIN32
        HWND parent = obtenir_hwnd_fenetre(args[0], ligne);
        wchar_t *texte = vers_large(args[1].comme.texte);
        int idx = creer_widget(parent, L"STATIC", texte, 0,
            (int)valeur_comme_reel(args[2]), (int)valeur_comme_reel(args[3]),
            (int)valeur_comme_reel(args[4]), (int)valeur_comme_reel(args[5]), ligne);
        free(texte);
        return valeur_entier(idx);
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_champ_texte") == 0) {
        if (nb_args != 5) interpreteur_erreur(ligne, "'fenetre_champ_texte' attend 5 arguments: fenetre, x, y, largeur, hauteur");
#ifdef _WIN32
        HWND parent = obtenir_hwnd_fenetre(args[0], ligne);
        int idx = creer_widget(parent, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL,
            (int)valeur_comme_reel(args[1]), (int)valeur_comme_reel(args[2]),
            (int)valeur_comme_reel(args[3]), (int)valeur_comme_reel(args[4]), ligne);
        return valeur_entier(idx);
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_lire_champ") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'fenetre_lire_champ' attend 1 argument: champ");
#ifdef _WIN32
        HWND hwnd = obtenir_hwnd_widget(args[0], ligne);
        int longueur = GetWindowTextLengthW(hwnd);
        wchar_t *large = malloc(sizeof(wchar_t) * (longueur + 1));
        GetWindowTextW(hwnd, large, longueur + 1);
        char *utf8 = vers_utf8(large);
        Valeur v = valeur_texte(utf8);
        free(large);
        free(utf8);
        return v;
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_definir_champ") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'fenetre_definir_champ' attend 2 arguments: champ, texte");
        exiger_texte(args[1], ligne, "fenetre_definir_champ");
#ifdef _WIN32
        HWND hwnd = obtenir_hwnd_widget(args[0], ligne);
        wchar_t *texte = vers_large(args[1].comme.texte);
        SetWindowTextW(hwnd, texte);
        free(texte);
        return valeur_rien();
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_sur_clic") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'fenetre_sur_clic' attend 2 arguments: bouton, fonction");
        if (args[1].type != V_FONCTION) interpreteur_erreur(ligne, "'fenetre_sur_clic' attend une fonction en second argument");
#ifdef _WIN32
        if (!valeur_est_nombre(args[0])) interpreteur_erreur(ligne, "identifiant de composant invalide");
        int idx = (int)valeur_comme_reel(args[0]);
        if (idx < 0 || idx >= g_nb_widgets_gui) interpreteur_erreur(ligne, "identifiant de composant invalide");
        g_widgets_gui[idx].callback = args[1];
        g_widgets_gui[idx].a_callback = TRUE;
        return valeur_rien();
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_executer") == 0) {
        if (nb_args != 0) interpreteur_erreur(ligne, "'fenetre_executer' n'attend aucun argument");
#ifdef _WIN32
        MSG msg;
        while (g_fenetres_actives_gui > 0 && GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return valeur_rien();
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    if (strcmp(nom, "fenetre_fermer") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'fenetre_fermer' attend 1 argument: fenetre");
#ifdef _WIN32
        HWND hwnd = obtenir_hwnd_fenetre(args[0], ligne);
        DestroyWindow(hwnd);
        return valeur_rien();
#else
        return erreur_windows_uniquement(ligne);
#endif
    }

    return valeur_rien();
}
