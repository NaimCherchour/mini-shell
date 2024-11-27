#ifndef HANDLER_H
#define HANDLER_H

/**
 * Lis la ligne de commande entrée par l'user et 
 * la découpe en 
 */
char** parse_command(char* prompt) ;

/** 
 * Exécute la ligne de commande `command`
 * commandes externes + internes
 */
void handle_command(char** command);
#endif