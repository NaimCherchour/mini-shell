#ifndef REDIRECTIONS_H
#define REDIRECTIONS_H

#include <stdbool.h>

/* On définit un enum pour les redirections  */
typedef enum {
    STDIN,        // <
    STDOUT,       // >
    STDOUT_TRUNC, // >|
    STDOUT_APPEND, // >>
    STDERR,        // 2>
    STDERR_TRUNC,  // 2>|
    STDERR_APPEND, // 2>>
    NONE          // Pas de redirection
} RedirectionType;

// Structure pour une redirection simple
typedef struct {
    RedirectionType type;  // Type de la redirection
    char * file;            // Nom du fichier associé
} Redirection;

/*
 * Détecte les redirections dans une commande donnée.
 * 
 * @param command    :   tableau de chaînes représentant les arguments de la commande
 * @param redirections : tableau où seront stockées les redirections détectées
 * @param max_redirections : taille maximale du tableau `redirections`.
 * @return               Nombre de redirections détectées, ou -1 en cas d'erreur
 */
int detect_redirections(char** command, Redirection* redirections, int max_redirections);

#endif