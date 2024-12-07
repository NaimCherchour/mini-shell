#ifndef HANDLER_H
#define HANDLER_H

/**
 * Lis la ligne de commande entrée par l'user et 
 * la découpe en 
 */
char** parse_input(char* prompt) ;

/**
 * Découpe les commandes successives separes par ; de la ligne de commande
 */
char*** cutout_commands(char** parsed_prompt);

void free_commands(char*** commands);

/** 
 * Exécute la ligne de commande `command`
 * commandes externes + internes
 */
int execute_command(char** command);

/**
 * Exécute les commandes
 */
int handle_commands(char*** commands);

#endif