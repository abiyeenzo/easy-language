CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=gnu11
LDFLAGS = -lm

SRC_COMMUNS = lexer.c analyseur.c valeur.c environnement.c interpreteur.c util.c

WINCC = x86_64-w64-mingw32-gcc

.PHONY: all clean windows linux

all: linux

linux: easy_language easy_debogueur

easy_language: main.c $(SRC_COMMUNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

easy_debogueur: debogueur_main.c debogueur.c $(SRC_COMMUNS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

windows: easy_language.exe easy_debogueur.exe

easy_language.exe: main.c $(SRC_COMMUNS)
	$(WINCC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

easy_debogueur.exe: debogueur_main.c debogueur.c $(SRC_COMMUNS)
	$(WINCC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f easy_language easy_language.exe easy_debogueur easy_debogueur.exe *.o
