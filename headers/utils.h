#ifndef UTILS_H
#define UTILS_H

/**
 * Vérifie la validité de la syntaxe for avec une commande simple 
 * Syntaxe : for F in REP { CMD }
 */
int for_syntax ( char** command );

int for_loop(char** command); // commande for et retourne la valeur de retour 

#endif