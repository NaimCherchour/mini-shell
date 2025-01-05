#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


#include "signal_handler.h"

// la var global  : flag si SIGINT a été reçu
 sig_atomic_t sigint_received = 0;


void handle_sigint(int sig) {
    sigint_received = 1;
}

// Fonction pour configurer les gestionnaires de signaux
void setup_signal_handlers() {
    struct sigaction sa = {};

    // Onn ignore SIGTERM dans le shell principal
    sa.sa_handler = SIG_IGN;
    sigaction(SIGTERM, &sa, NULL);

    // On capture SIGINT pour interrompre les commandes structurées
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint; // avec le flag
    sigaction(SIGINT, &sa, NULL);
}

//  Réinitialise les signaux dans les processus enfants ( cmds externes )
void reset_signals_in_child() {
    struct sigaction sa = {};
    sa.sa_handler = SIG_DFL; // comportement par défaut

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}