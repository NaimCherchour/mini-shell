#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

// On utilise une variable globale comme un flag qui nous indique si SIGINT a été reçu
// pour pourvoir l'utiliser dans les boucles
extern sig_atomic_t sigint_received;
//DOC : sig_atomic_t is an atomic entity even in the presence of asynchronous interrupts made by signals.


void setup_signal_handlers();
void reset_signals_in_child();

#endif
