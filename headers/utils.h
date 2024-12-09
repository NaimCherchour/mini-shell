#ifndef UTILS_H
#define UTILS_H

/**
 * Vérifie la validité de la syntaxe for avec une commande simple 
 * Syntaxe : for F in REP { CMD }
 */
int for_syntax (char** command, int optind);

char* replace_var_with_path(const char* str, const char* var, const char* replacement);

char** constructor(char** command, int optindex, char* full_path, char var, int extension);

int for_loop(char** command); // commande for et retourne la valeur de retour 

int if_else(char** command); // if TEST { CMD } else { CMD }

#endif