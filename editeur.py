import builtins
import contextlib
import os
import queue
import re
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox

from easy_language.analyseur import Analyseur
from easy_language.debogueur import ArretDebogueur, Debogueur
from easy_language.erreurs import ErreurEasyLang
from easy_language.interpreteur import Interpreteur
from easy_language.lexer import MOTS_CLES, Lexer
from easy_language.version import VERSION

MOTIF_MOT_CLE = r"\b(" + "|".join(sorted(MOTS_CLES, key=len, reverse=True)) + r")\b"
MOTIF_CHAINE = r'"([^"\\]|\\.)*"'
MOTIF_NOMBRE = r"\b\d+(\.\d+)?\b"
MOTIF_COMMENTAIRE = r"#.*"

# ordre = priorite d'affichage croissante (le dernier configure gagne sur les chevauchements)
COULEURS = {
    "mot_cle": "#c586c0",
    "nombre": "#b5cea8",
    "chaine": "#ce9178",
    "commentaire": "#6a9955",
}


class FluxSortie:
    """Objet fichier minimal qui pousse le texte ecrit dans une queue thread-safe."""

    def __init__(self, file_sortie):
        self.file_sortie = file_sortie

    def write(self, texte):
        if texte:
            self.file_sortie.put(texte)

    def flush(self):
        pass


class Editeur(tk.Tk):
    def __init__(self):
        super().__init__()
        self.titre_base = f"Easy Language {VERSION} - Editeur"
        self.title(f"{self.titre_base} [Nouveau fichier]")
        self.geometry("950x680")

        self.chemin_fichier = None
        self.thread_execution = None
        self.file_commandes = None  # utilisee pendant une session de debogage

        self._construire_menu()
        self._construire_widgets()
        self._colorer_syntaxe()

    # --- construction de l'interface ---

    def _construire_menu(self):
        barre = tk.Menu(self)

        menu_fichier = tk.Menu(barre, tearoff=0)
        menu_fichier.add_command(label="Nouveau", command=self.nouveau, accelerator="Ctrl+N")
        menu_fichier.add_command(label="Ouvrir...", command=self.ouvrir, accelerator="Ctrl+O")
        menu_fichier.add_command(label="Enregistrer", command=self.enregistrer, accelerator="Ctrl+S")
        menu_fichier.add_command(label="Enregistrer sous...", command=self.enregistrer_sous)
        menu_fichier.add_separator()
        menu_fichier.add_command(label="Quitter", command=self.quit)
        barre.add_cascade(label="Fichier", menu=menu_fichier)

        menu_executer = tk.Menu(barre, tearoff=0)
        menu_executer.add_command(label="Lancer", command=self.lancer, accelerator="F5")
        menu_executer.add_command(label="Deboguer", command=self.deboguer, accelerator="F6")
        barre.add_cascade(label="Executer", menu=menu_executer)

        menu_aide = tk.Menu(barre, tearoff=0)
        menu_aide.add_command(label="A propos", command=self.a_propos)
        barre.add_cascade(label="Aide", menu=menu_aide)

        self.config(menu=barre)

        self.bind("<Control-n>", lambda e: self.nouveau())
        self.bind("<Control-o>", lambda e: self.ouvrir())
        self.bind("<Control-s>", lambda e: self.enregistrer())
        self.bind("<F5>", lambda e: self.lancer())
        self.bind("<F6>", lambda e: self.deboguer())

    def _construire_widgets(self):
        cadre_principal = tk.PanedWindow(self, orient=tk.VERTICAL)
        cadre_principal.pack(fill=tk.BOTH, expand=True)

        cadre_edition = tk.Frame(cadre_principal)
        self.numeros_ligne = tk.Text(
            cadre_edition, width=4, padx=4, takefocus=0, border=0,
            state="disabled", background="#f0f0f0",
        )
        self.numeros_ligne.pack(side=tk.LEFT, fill=tk.Y)

        self.texte = tk.Text(cadre_edition, wrap="none", undo=True, font=("Consolas", 11))
        self.texte.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        defiler = tk.Scrollbar(cadre_edition, command=self._defiler)
        defiler.pack(side=tk.RIGHT, fill=tk.Y)
        self.texte.configure(yscrollcommand=defiler.set)
        cadre_principal.add(cadre_edition, height=420)

        for tag, couleur in COULEURS.items():
            self.texte.tag_configure(tag, foreground=couleur)
        self.texte.bind("<KeyRelease>", self._sur_modification)

        cadre_bas = tk.Frame(cadre_principal)

        tk.Label(cadre_bas, text="Entree standard (une valeur par ligne, pour 'demande')").pack(anchor="w")
        self.entree_stdin = tk.Text(cadre_bas, height=3, font=("Consolas", 10))
        self.entree_stdin.pack(fill=tk.X)

        tk.Label(cadre_bas, text="Sortie").pack(anchor="w")
        self.sortie = tk.Text(
            cadre_bas, height=12, state="disabled", background="#1e1e1e",
            foreground="#dcdcdc", font=("Consolas", 10),
        )
        self.sortie.pack(fill=tk.BOTH, expand=True)

        cadre_commande = tk.Frame(cadre_bas)
        tk.Label(cadre_commande, text="Commande debogueur:").pack(side=tk.LEFT)
        self.entree_commande = tk.Entry(cadre_commande, state="disabled")
        self.entree_commande.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=4)
        self.entree_commande.bind("<Return>", self._envoyer_commande)
        cadre_commande.pack(fill=tk.X)

        cadre_principal.add(cadre_bas)

        self._mettre_a_jour_numeros_ligne()

    def _defiler(self, *args):
        self.texte.yview(*args)
        self.numeros_ligne.yview(*args)

    # --- coloration syntaxique ---

    def _sur_modification(self, event=None):
        self._colorer_syntaxe()
        self._mettre_a_jour_numeros_ligne()

    def _mettre_a_jour_numeros_ligne(self):
        nb_lignes = int(self.texte.index("end-1c").split(".")[0])
        contenu = "\n".join(str(i) for i in range(1, nb_lignes + 1))
        self.numeros_ligne.configure(state="normal")
        self.numeros_ligne.delete("1.0", "end")
        self.numeros_ligne.insert("1.0", contenu)
        self.numeros_ligne.configure(state="disabled")

    def _colorer_syntaxe(self):
        for tag in COULEURS:
            self.texte.tag_remove(tag, "1.0", "end")

        contenu = self.texte.get("1.0", "end-1c")
        for motif, tag in (
            (MOTIF_MOT_CLE, "mot_cle"),
            (MOTIF_NOMBRE, "nombre"),
            (MOTIF_CHAINE, "chaine"),
            (MOTIF_COMMENTAIRE, "commentaire"),
        ):
            for correspondance in re.finditer(motif, contenu):
                debut = f"1.0+{correspondance.start()}c"
                fin = f"1.0+{correspondance.end()}c"
                self.texte.tag_add(tag, debut, fin)

    # --- gestion de fichiers ---

    def nouveau(self):
        self.texte.delete("1.0", "end")
        self.chemin_fichier = None
        self.title(f"{self.titre_base} [Nouveau fichier]")
        self._sur_modification()

    def ouvrir(self):
        chemin = filedialog.askopenfilename(filetypes=[("Easy Language", "*.elg"), ("Tous les fichiers", "*.*")])
        if not chemin:
            return
        with open(chemin, "r", encoding="utf-8") as f:
            contenu = f.read()
        self.texte.delete("1.0", "end")
        self.texte.insert("1.0", contenu)
        self.chemin_fichier = chemin
        self.title(f"{self.titre_base} [{chemin}]")
        self._sur_modification()

    def enregistrer(self):
        if not self.chemin_fichier:
            return self.enregistrer_sous()
        with open(self.chemin_fichier, "w", encoding="utf-8") as f:
            f.write(self.texte.get("1.0", "end-1c"))
        return True

    def enregistrer_sous(self):
        chemin = filedialog.asksaveasfilename(defaultextension=".elg", filetypes=[("Easy Language", "*.elg")])
        if not chemin:
            return False
        self.chemin_fichier = chemin
        self.title(f"{self.titre_base} [{chemin}]")
        return self.enregistrer()

    # --- execution et debogage ---

    def lancer(self):
        self._demarrer_execution(deboguer=False)

    def deboguer(self):
        self._demarrer_execution(deboguer=True)

    def _demarrer_execution(self, deboguer):
        if self.thread_execution is not None and self.thread_execution.is_alive():
            messagebox.showinfo("Easy Language", "Une execution est deja en cours.")
            return

        if not self.enregistrer():
            return

        self._effacer_sortie()
        source = self.texte.get("1.0", "end-1c")
        dossier = os.path.dirname(os.path.abspath(self.chemin_fichier))

        self.file_sortie = queue.Queue()
        self.file_commandes = queue.Queue() if deboguer else None
        if deboguer:
            self.entree_commande.configure(state="normal")
            self.entree_commande.focus_set()

        self.thread_execution = threading.Thread(
            target=self._executer_dans_thread, args=(source, dossier, deboguer), daemon=True,
        )
        self.thread_execution.start()
        self.after(50, self._lire_file_sortie)

    def _executer_dans_thread(self, source, dossier, deboguer):
        ancien_input = builtins.input
        flux = FluxSortie(self.file_sortie)

        if deboguer:
            def entree_debogueur(invite=""):
                if invite:
                    self.file_sortie.put(invite + "\n")
                return self.file_commandes.get()

            builtins.input = entree_debogueur
        else:
            lignes_stdin = iter(self.entree_stdin.get("1.0", "end-1c").split("\n"))

            def entree_normale(invite=""):
                if invite:
                    self.file_sortie.put(invite)
                try:
                    return next(lignes_stdin)
                except StopIteration:
                    raise EOFError("plus de lignes disponibles dans 'entree standard'")

            builtins.input = entree_normale

        try:
            with contextlib.redirect_stdout(flux):
                jetons = Lexer(source).tokeniser()
                programme = Analyseur(jetons).analyser()
                interpreteur = Interpreteur()
                if deboguer:
                    Debogueur(interpreteur, source.split("\n"))
                interpreteur.executer(programme, dossier)
            self.file_sortie.put("\n=== termine ===\n")
        except ArretDebogueur:
            self.file_sortie.put("\n=== execution arretee ===\n")
        except ErreurEasyLang as e:
            self.file_sortie.put(f"\nErreur ligne {e.ligne}: {e.message}\n")
        except Exception as e:
            self.file_sortie.put(f"\nErreur interne: {e}\n")
        finally:
            builtins.input = ancien_input
            self.file_sortie.put(None)

    def _envoyer_commande(self, event=None):
        if self.file_commandes is None:
            return
        commande = self.entree_commande.get()
        self.entree_commande.delete(0, "end")
        self._ecrire_sortie(f"(debogueur) {commande}\n")
        self.file_commandes.put(commande)

    def _lire_file_sortie(self):
        try:
            while True:
                item = self.file_sortie.get_nowait()
                if item is None:
                    self.entree_commande.configure(state="disabled")
                    self.file_commandes = None
                    return
                self._ecrire_sortie(item)
        except queue.Empty:
            self.after(50, self._lire_file_sortie)

    def _ecrire_sortie(self, texte):
        self.sortie.configure(state="normal")
        self.sortie.insert("end", texte)
        self.sortie.see("end")
        self.sortie.configure(state="disabled")

    def _effacer_sortie(self):
        self.sortie.configure(state="normal")
        self.sortie.delete("1.0", "end")
        self.sortie.configure(state="disabled")

    def a_propos(self):
        messagebox.showinfo(
            "A propos",
            f"Easy Language {VERSION}\n\n"
            "Langage de programmation interprete en francais.\n"
            "Fichiers .elg\n\n"
            "https://github.com/abiyeenzo/easy-language",
        )


def main():
    app = Editeur()
    if len(sys.argv) > 1:
        chemin = sys.argv[1]
        if os.path.isfile(chemin):
            with open(chemin, "r", encoding="utf-8") as f:
                contenu = f.read()
            app.texte.delete("1.0", "end")
            app.texte.insert("1.0", contenu)
            app.chemin_fichier = chemin
            app.title(f"{app.titre_base} [{chemin}]")
            app._sur_modification()
    app.mainloop()


if __name__ == "__main__":
    main()
