from .erreurs import ErreurLexicale

MOTS_CLES = {
    "soit", "affiche", "demande",
    "si", "alors", "sinon",
    "tantque", "pour", "de", "jusqua", "pas", "chaque", "dans",
    "fonction", "retourne",
    "vrai", "faux", "rien",
    "et", "ou", "non",
    "arrete", "continue",
    "importe", "comme",
}

SYMBOLES = [
    ("==", "EGAL_EGAL"), ("!=", "DIFFERENT"),
    ("<=", "INF_EGAL"), (">=", "SUP_EGAL"),
    ("+", "PLUS"), ("-", "MOINS"), ("*", "FOIS"), ("/", "DIVISE"), ("%", "MODULO"),
    ("=", "EGAL"), ("<", "INF"), (">", "SUP"),
    ("(", "PARO"), (")", "PARF"),
    ("[", "CROCHET_O"), ("]", "CROCHET_F"),
    (":", "DEUX_POINTS"), (".", "POINT"),
]


class Jeton:
    def __init__(self, type_, valeur, ligne):
        self.type = type_
        self.valeur = valeur
        self.ligne = ligne

    def __repr__(self):
        return f"Jeton({self.type}, {self.valeur!r})"


class Lexer:
    def __init__(self, source):
        self.lignes = source.split("\n")
        self.jetons = []
        self.pile_indentation = [0]

    def tokeniser(self):
        for numero, ligne_brute in enumerate(self.lignes, start=1):
            self._traiter_ligne(ligne_brute, numero)

        while len(self.pile_indentation) > 1:
            self.pile_indentation.pop()
            self.jetons.append(Jeton("DEDENT", None, len(self.lignes) + 1))

        self.jetons.append(Jeton("FIN", None, len(self.lignes) + 1))
        return self.jetons

    def _traiter_ligne(self, ligne_brute, numero):
        if "\t" in ligne_brute.split("#")[0]:
            raise ErreurLexicale(
                "les tabulations ne sont pas autorisees pour l'indentation, utilisez des espaces",
                numero,
            )

        sans_espaces = ligne_brute.lstrip(" ")
        indentation = len(ligne_brute) - len(sans_espaces)
        contenu = sans_espaces.split("#", 1)[0].rstrip()

        if contenu == "":
            return

        if indentation > self.pile_indentation[-1]:
            self.pile_indentation.append(indentation)
            self.jetons.append(Jeton("INDENT", indentation, numero))
        else:
            while indentation < self.pile_indentation[-1]:
                self.pile_indentation.pop()
                self.jetons.append(Jeton("DEDENT", None, numero))
            if indentation != self.pile_indentation[-1]:
                raise ErreurLexicale("indentation incoherente", numero)

        self._tokeniser_contenu(contenu, numero)
        self.jetons.append(Jeton("NOUVELLE_LIGNE", None, numero))

    def _tokeniser_contenu(self, contenu, numero):
        i = 0
        n = len(contenu)
        while i < n:
            c = contenu[i]

            if c == " ":
                i += 1
                continue

            if c == '"':
                j = i + 1
                valeur = []
                while j < n and contenu[j] != '"':
                    if contenu[j] == "\\" and j + 1 < n:
                        suivant = contenu[j + 1]
                        correspondance = {"n": "\n", "t": "\t", '"': '"', "\\": "\\"}
                        valeur.append(correspondance.get(suivant, suivant))
                        j += 2
                    else:
                        valeur.append(contenu[j])
                        j += 1
                if j >= n:
                    raise ErreurLexicale("chaine de texte non terminee", numero)
                self.jetons.append(Jeton("TEXTE", "".join(valeur), numero))
                i = j + 1
                continue

            if c.isdigit():
                j = i
                a_point = False
                while j < n and (contenu[j].isdigit() or (contenu[j] == "." and not a_point)):
                    if contenu[j] == ".":
                        a_point = True
                    j += 1
                brut = contenu[i:j]
                valeur = float(brut) if a_point else int(brut)
                self.jetons.append(Jeton("NOMBRE", valeur, numero))
                i = j
                continue

            if c.isalpha() or c == "_":
                j = i
                while j < n and (contenu[j].isalnum() or contenu[j] == "_"):
                    j += 1
                mot = contenu[i:j]
                if mot in MOTS_CLES:
                    self.jetons.append(Jeton(mot.upper(), mot, numero))
                else:
                    self.jetons.append(Jeton("IDENT", mot, numero))
                i = j
                continue

            trouve = False
            for symbole, nom in SYMBOLES:
                if contenu.startswith(symbole, i):
                    self.jetons.append(Jeton(nom, symbole, numero))
                    i += len(symbole)
                    trouve = True
                    break
            if trouve:
                continue

            raise ErreurLexicale(f"caractere inattendu: {c!r}", numero)
