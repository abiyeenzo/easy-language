#include "natifs_reseau.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET DescripteurSocket;
#define SOCKET_INVALIDE INVALID_SOCKET
#define FERMER_SOCKET closesocket
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int DescripteurSocket;
#define SOCKET_INVALIDE (-1)
#define FERMER_SOCKET close
#endif

#include "interpreteur.h"

static int winsock_pret = 0;

static void assurer_winsock(void) {
#ifdef _WIN32
    if (!winsock_pret) {
        WSADATA donnees;
        WSAStartup(MAKEWORD(2, 2), &donnees);
        winsock_pret = 1;
    }
#else
    (void)winsock_pret;
#endif
}

int natifs_reseau_est(const char *nom) {
    static const char *noms[] = {"reseau_connecter", "reseau_envoyer", "reseau_recevoir", "reseau_fermer"};
    for (size_t i = 0; i < sizeof(noms) / sizeof(noms[0]); i++) {
        if (strcmp(nom, noms[i]) == 0) return 1;
    }
    return 0;
}

Valeur natifs_reseau_appeler(const char *nom, Valeur *args, int nb_args, int ligne) {
    assurer_winsock();

    if (strcmp(nom, "reseau_connecter") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'reseau_connecter' attend 2 arguments: hote et port");
        if (args[0].type != V_TEXTE) interpreteur_erreur(ligne, "reseau_connecter() attend un hote en texte");
        if (!valeur_est_nombre(args[1])) interpreteur_erreur(ligne, "reseau_connecter() attend un port numerique");

        char port_texte[16];
        snprintf(port_texte, sizeof(port_texte), "%lld", (long long)valeur_comme_reel(args[1]));

        struct addrinfo indices;
        memset(&indices, 0, sizeof(indices));
        indices.ai_family = AF_UNSPEC;
        indices.ai_socktype = SOCK_STREAM;

        struct addrinfo *resultats = NULL;
        if (getaddrinfo(args[0].comme.texte, port_texte, &indices, &resultats) != 0) {
            return valeur_entier(-1);
        }

        DescripteurSocket sock = SOCKET_INVALIDE;
        for (struct addrinfo *p = resultats; p != NULL; p = p->ai_next) {
            sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock == SOCKET_INVALIDE) continue;
            if (connect(sock, p->ai_addr, (int)p->ai_addrlen) == 0) break;
            FERMER_SOCKET(sock);
            sock = SOCKET_INVALIDE;
        }
        freeaddrinfo(resultats);

        if (sock == SOCKET_INVALIDE) return valeur_entier(-1);
        return valeur_entier((int64_t)sock);
    }

    if (strcmp(nom, "reseau_envoyer") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'reseau_envoyer' attend 2 arguments: socket et texte");
        if (!valeur_est_nombre(args[0])) interpreteur_erreur(ligne, "reseau_envoyer() attend un socket (entier)");
        char *contenu = valeur_formater(args[1]);
        DescripteurSocket sock = (DescripteurSocket)(int64_t)valeur_comme_reel(args[0]);
        long envoye = send(sock, contenu, (int)strlen(contenu), 0);
        free(contenu);
        return valeur_entier(envoye);
    }

    if (strcmp(nom, "reseau_recevoir") == 0) {
        if (nb_args != 2) interpreteur_erreur(ligne, "'reseau_recevoir' attend 2 arguments: socket et taille max");
        if (!valeur_est_nombre(args[0]) || !valeur_est_nombre(args[1])) {
            interpreteur_erreur(ligne, "reseau_recevoir() attend des arguments numeriques");
        }
        DescripteurSocket sock = (DescripteurSocket)(int64_t)valeur_comme_reel(args[0]);
        long max_octets = (long)valeur_comme_reel(args[1]);
        if (max_octets <= 0 || max_octets > 65536) max_octets = 4096;
        char *tampon = malloc((size_t)max_octets + 1);
        long recu = recv(sock, tampon, (int)max_octets, 0);
        if (recu <= 0) { free(tampon); return valeur_rien(); /* connexion fermee ou erreur */ }
        tampon[recu] = '\0';
        Valeur v = valeur_texte(tampon);
        free(tampon);
        return v;
    }

    if (strcmp(nom, "reseau_fermer") == 0) {
        if (nb_args != 1) interpreteur_erreur(ligne, "'reseau_fermer' attend 1 argument");
        if (!valeur_est_nombre(args[0])) interpreteur_erreur(ligne, "reseau_fermer() attend un socket (entier)");
        DescripteurSocket sock = (DescripteurSocket)(int64_t)valeur_comme_reel(args[0]);
        FERMER_SOCKET(sock);
        return valeur_rien();
    }

    return valeur_rien();
}
