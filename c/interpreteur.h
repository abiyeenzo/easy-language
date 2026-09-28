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

/* Affiche "Erreur ligne N: msg" sur stderr et quitte (exit 1), comme
   toutes les erreurs d'execution du langage. Exposee aux modules de
   natifs (natifs_*.c) pour rester coherente avec les erreurs du coeur. */
void interpreteur_erreur(int ligne, const char *msg);

/* Appelle une Valeur de type V_FONCTION avec les arguments donnes, comme
   le ferait un appel normal dans le script. Exposee pour les natifs qui
   ont besoin de rappeler dans le script (callbacks) : voir natifs_gui.c,
   ou une fonction Easy Language peut etre enregistree comme gestionnaire
   du clic sur un bouton. Erreur d'execution si 'fonction' n'est pas une
   V_FONCTION. */
Valeur interpreteur_appeler(Valeur fonction, Valeur *args, int nb_args, int ligne);

#endif
