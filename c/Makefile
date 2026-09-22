CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=gnu11
LDFLAGS = -lm

SRC_COMMUNS = lexer.c analyseur.c valeur.c environnement.c interpreteur.c util.c

WINCC = x86_64-w64-mingw32-gcc

.PHONY: all clean windows linux tests

all: linux

linux: easy_language easy_debogueur

easy_language: main.c $(SRC_COMMUNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

easy_debogueur: debogueur_main.c debogueur.c $(SRC_COMMUNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# L'editeur utilise l'API Win32 directement: uniquement compilable pour Windows.
windows: easy_language.exe easy_debogueur.exe easy_editeur.exe

easy_language.exe: main.c $(SRC_COMMUNS)
	$(WINCC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

easy_debogueur.exe: debogueur_main.c debogueur.c $(SRC_COMMUNS)
	$(WINCC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

easy_editeur.exe: editeur.c
	$(WINCC) $(CFLAGS) -municode -mwindows -o $@ $^ -lcomdlg32 -lgdi32 -luser32 -lkernel32

tests: easy_language easy_debogueur
	$(CC) $(CFLAGS) -o tests/executer_tests tests/test_interpreteur.c tests/test_debogueur.c tests/aide.c
	cd tests && ./executer_tests

clean:
	rm -f easy_language easy_language.exe easy_debogueur easy_debogueur.exe easy_editeur.exe *.o
	rm -f tests/executer_tests
