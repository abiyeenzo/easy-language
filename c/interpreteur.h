#ifndef INTERPRETEUR_H
#define INTERPRETEUR_H

#include "noeuds.h"
#include "environnement.h"

typedef void (*HookInstruction)(Noeud *instruction, Environnement *env);

/* Appele juste avant chaque instruction executee si defini (NULL par
   defaut). Utilise par le debogueur pour le pas-a-pas et les points
   d'arret, sans toucher au chemin normal d'execution. */
void interpreteur_definir_hook(HookInstruction hook);

/* Repertoire de base utilise pour resoudre les chemins relatifs d'un
   'importe'. A definir avant d'appeler interpreteur_executer (main.c /
   debogueur_main.c le derivent du fichier passe en argument). */
void interpreteur_definir_dossier(const char *dossier);

void interpreteur_executer(Noeud *programme, Environnement *globales);

#endif
