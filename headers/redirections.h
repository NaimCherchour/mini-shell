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

/*
 * duplique les descripteurs de fichiers pour sauvegarder leur état initial afin de les restorer
 * 
 * @param fds Tableau de trois descripteurs (stdin, stdout, stderr).
 * @return    0 en cas de succès, -1 en cas d'échec.
 */
int save_fds(int fds[3]) ;

/*
 * Restaure les descripteurs de fichiers à leur état initial.
 * 
 * @param fds Tableau des trois descripteurs sauvegardés
 * @return    0 en cas de succès, -1 en cas d'échec
 */
int restore_fds(int fds[3]);

/*
 * Applique une redirection spécifique.
 * 
 * @param redir une structure représentant la redirection à appliquer
 * @return      true si la redirection est appliquée, false sinon
 */
bool apply_redirection(Redirection redir);

#endif