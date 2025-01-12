#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/limits.h>


#include "../headers/internals.h"
#include "../headers/prompt.h" // pour utiliser la variable last_status ie valeur de retour 


char rep_precedent[PATH_MAX]; // on sauvegarde le répertoire précédent


int nb_arguments (char **args ) {
    // Calcul le nombre d'arguments
    int argc = 0;
    while (args[argc] != NULL) {
        argc++;
    }
    return argc;
}

int cd (char **args) {

    char *rep_destinataire;
    char rep_courant[PATH_MAX];

    int argc = nb_arguments(args); // calculer le nombre d'arguments 

    if (argc > 2) {
        write(STDERR_FILENO, "cd: too many arguments\n", 23);
        return 1;
    }

    if (args[1] == NULL) {     // Si l'utilisateur entre que "cd", on va vers le répertoire HOME
        rep_destinataire = getenv("HOME");
        if (rep_destinataire == NULL) {
            write(STDERR_FILENO, "cd: HOME non définie \n",22);
            return 1;
        }
    } else if (strcmp(args[1], "-") == 0) {     // Si l'argument est "-", on va vers le répertoire précédent
        if (rep_precedent[0] == '\0') {
            write(STDERR_FILENO, "cd: répertoire précédent non défini \n",41);
            return 1;
        }
        rep_destinataire = rep_precedent;
    }
    // Sinon, on va vers le répertoire spécifié par l'utilisateur
    else {
        rep_destinataire = args[1];
    }

    // On sauvegarde le répertoire courant avant de changer de répertoire
    if (getcwd(rep_courant, sizeof(rep_courant)) == NULL) {
        perror("getcwd");
        return 1;
    }
    // Changement de répertoire
    if (chdir(rep_destinataire) != 0) {
        perror("cd");
        return 1;
    }

    // Mise à jour du répertoire précédent
    if (strncpy(rep_precedent, rep_courant, sizeof(rep_precedent)) == NULL) {
    perror("strncpy");
    return 1;
    }


    return 0;
}

int ftype(char **args) {
    struct stat file_stat;

    if (args[1] == NULL) {
        write(STDERR_FILENO, "ftype: paramètre manquant\n",27);
        return 1;
    }
    if (lstat(args[1], &file_stat) == -1) {
        perror("ftype");
        return 1;
    }
    
    const char *type_message = NULL; // le type affiché
    if (S_ISDIR(file_stat.st_mode)) {
        type_message = "directory\n";
    } else if (S_ISREG(file_stat.st_mode)) {
        type_message = "regular file\n";
    } else if (S_ISLNK(file_stat.st_mode)) {
        type_message = "symbolic link\n";
    } else if (S_ISFIFO(file_stat.st_mode)) {
        type_message = "named pipe\n";
    } else {
        type_message = "other\n";
    }

    write(STDOUT_FILENO, type_message, strlen(type_message));
    return 0;
}

int pwd(char **args) {

    int argc = nb_arguments(args);
    if (argc > 1) {
        write(STDERR_FILENO, "pwd: too many arguments\n", 24);
        return 1;
    }

    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char buffer[PATH_MAX+1];
        snprintf(buffer, sizeof(buffer), "%s\n", cwd);
        write(STDOUT_FILENO, buffer, strlen(buffer));  // Affiche le répertoire courant
        return 0;
    } else {
        perror("pwd");  // Affiche une erreur si `getcwd` échoue
        return 1;
    }
}

int exit_shell(char **args) {
    int argc = 0;

    // Vérifier si args est NULL (cas d'un CTRL+D ou d'un appel exit_shell(NULL) )
    if (args == NULL) {
        exit(last_status);
    }

    // Compter le nombre d'arguments
    argc = nb_arguments(args);

    switch (argc) {
        case 1:
            // Aucun argument, quitter avec le dernier code de retour de la commande
            exit(last_status); // Utilise last_status qui a été mis à jour par les commandes précédentes

        case 2:
            // Un argument, vérifier si c'est un entier valide pour le code de retour
            int value;
            if (sscanf(args[1], "%d", &value) <= 0) {
                write(STDERR_FILENO, "Usage: exit [VAL]\n", 18);
                errno = EINVAL;  // Si l'argument n'est pas un entier valide
                return 1;
            }
            exit(value); // Quitter avec le code de retour spécifié

        default:
            // Trop d'arguments, erreur
            errno = E2BIG; 
            write(STDERR_FILENO, "Usage: exit [VAL]\n", 18);
            return 1;
    }
}
