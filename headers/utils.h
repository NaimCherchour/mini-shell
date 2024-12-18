#ifndef UTILS_H
#define UTILS_H

// Prototypes des fonctions utilisées dans ce fichier 

int execute_block(char *command);

char* replace_var_with_path(const char* str, const char* var, const char* replacement);

int for_loop(char** command); // commande for et retourne la valeur de retour 

int if_else(char** command); // if TEST { CMD } else { CMD }

#endif