#ifndef PROMPT_H
#define PROMPT_H

#include <stddef.h>

// Constantes pour les couleurs
 /* \001 et \002 indiquent à readline de ne pas considérer les caractères non imprimables 
    dans le calcul de la longueur de l'affichage */

#define COLOR_GREEN "\001\033[32m\002" // Vert: succès
#define COLOR_RED "\001\033[91m\002"   // Rouge: échec 
#define COLOR_BLUE "\001\033[34m\002"  // Bleu: chemin courant
#define COLOR_RESET "\001\033[00m\002" // Retour à la normale

// Valeur spéciale pour interruption par signal définie dans prompt.c
#define STATUS_SIGNAL 255

// Variable globale pour stocker la dernière valeur de retour et initialisée à 0 
extern int last_status;

/**
 * Le prompt est limité à 30 caractères et inclut :
 * - Une bascule de couleur (rouge ou vert) selon la valeur de last_status
 * - Entre crochets: la valeur de retour de la dernière commande ou "SIG"
 * - Une bascule de couleur bleu pour le chemin courant
 * - Le chemin courant tronqué à gauche si nécessaire
 * - Le symbole `$` suivi d'un espace
 *
 * @return une chaîne de caractères allouée dynamiquement contenant le prompt
 *         On libérer la mémoire après utilisation
 */
char *generate_prompt();

#endif