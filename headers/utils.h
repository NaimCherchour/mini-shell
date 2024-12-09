#ifndef UTILS_H
#define UTILS_H

/**
 * Vérifie la validité de la syntaxe for avec une commande simple 
 * for F in REP [-A] [-r] [-e EXT] [-t TYPE] [-p MAX] { CMD }
 */
int for_syntax (char** command, int optind);

char* replace_var_with_path(const char* str, const char* var, const char* replacement);

char** constructor(char** command, int optindex, char* full_path, char var, int extension);

int do_for(char** command, int optindex, char var, char* full_path, int extension);

int browse_directory(const char *directory, int hidden, int recursive, int extension, char* EXT, int type, char TYPE, char var, char **command, int optindex, int return_val);

int for_loop(char** command); // commande for et retourne la valeur de retour 

int if_else(char** command); // if TEST { CMD } else { CMD }

#endif