#include "bibliotheque_index.h"

#include <string.h>

const EntreeBibliotheque BIBLIOTHEQUE_INDEX[] = {
    {"math", "Fonctions mathematiques : racine, puissance, sin/cos/tan, abs, plancher/plafond/arrondi, log/exp, alea, min/max, PI, E."},
    {"os", "Fichiers et systeme : fichier_existe, lire/ecrire/ajouter/supprimer_fichier, repertoire_courant, variable_environnement, horodatage, dormir."},
    {"reseau", "Client TCP basique : connecter, envoyer, recevoir, fermer."},
    {"gui", "Interface graphique (Windows uniquement) : boites de dialogue (message, question) et vraies fenetres/composants (creer, bouton, etiquette, champ_texte, sur_clic, executer)."},
    {"texte", "Manipulation de chaines : decouper, joindre, remplacer, majuscules/minuscules, rogner, commence_par/finit_par, sous_texte, inverse, position, contient_texte."},
    {"temps", "Date et heure : maintenant, annee/mois/jour/heure/minute/seconde, jour_semaine, formater (codes strftime)."},
};

const int BIBLIOTHEQUE_INDEX_COMPTE = sizeof(BIBLIOTHEQUE_INDEX) / sizeof(BIBLIOTHEQUE_INDEX[0]);

int bibliotheque_index_connu(const char *nom) {
    for (int i = 0; i < BIBLIOTHEQUE_INDEX_COMPTE; i++) {
        if (strcmp(BIBLIOTHEQUE_INDEX[i].nom, nom) == 0) return 1;
    }
    return 0;
}
