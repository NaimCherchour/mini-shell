#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

// On utilise une variable globale comme un flag qui nous indique si SIGINT a été reçu
// pour pourvoir l'utiliser dans les boucles
extern sig_atomic_t sigint_received;
//DOC : sig_atomic_t is an atomic entity even in the presence of asynchronous interrupts made by signals.

// Fonction qui initialise les gestionnaires de signaux pour SIGINT et SIGCHLD
// SIGTERM est ignoré et SIGINT est capturé par fsh
void setup_signal_handlers();

// Fonction qui réinitialise les gestionnaires de signaux appelé dans les commandes externes
void reset_signals_in_child();

#endif
