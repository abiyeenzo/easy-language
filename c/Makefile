CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=gnu11
LDFLAGS = -lm

SRC_COMMUNS = lexer.c analyseur.c valeur.c environnement.c interpreteur.c util.c

WINCC = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres

.PHONY: all clean windows linux tests

all: linux

linux: easy_language easy_debogueur

easy_language: main.c $(SRC_COMMUNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

easy_debogueur: debogueur_main.c debogueur.c $(SRC_COMMUNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# L'editeur utilise l'API Win32 directement: uniquement compilable pour Windows.
# Les trois executables embarquent chacun le logo (packaging/icone.ico) et des
# metadonnees d'auteur (societe, copyright) via leur propre ressource .rc,
# visibles dans l'Explorateur (icone) et l'onglet Details des proprietes du
# fichier (auteur/copyright).
windows: easy_language.exe easy_debogueur.exe easy_editeur.exe

ressources_interpreteur.o: ressources_interpreteur.rc ressources.h ../packaging/icone.ico
	$(WINDRES) -o $@ ressources_interpreteur.rc

ressources_debogueur.o: ressources_debogueur.rc ressources.h ../packaging/icone.ico
	$(WINDRES) -o $@ ressources_debogueur.rc

ressources_editeur.o: ressources_editeur.rc ressources.h ../packaging/icone.ico
	$(WINDRES) -o $@ ressources_editeur.rc

easy_language.exe: main.c $(SRC_COMMUNS) ressources_interpreteur.o
	$(WINCC) $(CFLAGS) -o $@ main.c $(SRC_COMMUNS) ressources_interpreteur.o $(LDFLAGS)

easy_debogueur.exe: debogueur_main.c debogueur.c $(SRC_COMMUNS) ressources_debogueur.o
	$(WINCC) $(CFLAGS) -o $@ debogueur_main.c debogueur.c $(SRC_COMMUNS) ressources_debogueur.o $(LDFLAGS)

easy_editeur.exe: editeur.c ressources_editeur.o
	$(WINCC) $(CFLAGS) -municode -mwindows -o $@ editeur.c ressources_editeur.o -lcomdlg32 -lgdi32 -luser32 -lkernel32

tests: easy_language easy_debogueur
	$(CC) $(CFLAGS) -o tests/executer_tests tests/test_interpreteur.c tests/test_debogueur.c tests/aide.c
	cd tests && ./executer_tests

clean:
	rm -f easy_language easy_language.exe easy_debogueur easy_debogueur.exe easy_editeur.exe *.o
	rm -f tests/executer_tests
