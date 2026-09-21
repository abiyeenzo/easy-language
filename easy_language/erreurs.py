class ErreurEasyLang(Exception):
    def __init__(self, message, ligne=None):
        self.message = message
        self.ligne = ligne
        if ligne is not None:
            super().__init__(f"ligne {ligne}: {message}")
        else:
            super().__init__(message)


class ErreurLexicale(ErreurEasyLang):
    pass


class ErreurSyntaxe(ErreurEasyLang):
    pass


class ErreurExecution(ErreurEasyLang):
    pass
