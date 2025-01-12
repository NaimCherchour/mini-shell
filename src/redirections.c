#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include <linux/limits.h> // Pour #define PATH_MAX 4096
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h> // Pour open


#include "../headers/redirections.h"
#include "../headers/handler.h" 
#include "../headers/prompt.h" // pour last_status

// Analyse une commande pour détecter les redirections
int detect_redirections(char** command, Redirection* redirections, int max_redirections) {
    int redir_count = 0;
    int i = 0;

    while (command[i] != NULL) {
        if (redir_count >= max_redirections) {
            write(STDERR_FILENO,"Nombre maximal de redirections atteint\n",40);
            return redir_count;
        }

        // Détecte le type de redirection
        if (strcmp(command[i], "<") == 0) {
            redirections[redir_count].type = STDIN;
        } else if (strcmp(command[i], ">") == 0) {
            redirections[redir_count].type = STDOUT;
        } else if (strcmp(command[i], ">|") == 0) {
            redirections[redir_count].type = STDOUT_TRUNC;
        } else if (strcmp(command[i], ">>") == 0) {
            redirections[redir_count].type = STDOUT_APPEND;
        } else if (strcmp(command[i], "2>") == 0) {
            redirections[redir_count].type = STDERR;
        } else if (strcmp(command[i], "2>|") == 0) {
            redirections[redir_count].type = STDERR_TRUNC;
        } else if (strcmp(command[i], "2>>") == 0) {
            redirections[redir_count].type = STDERR_APPEND;
        } else {
            i++;
            continue;
        }

        // On vérifie le fichier cible
        if (command[i + 1] == NULL) {
            write(STDERR_FILENO, "Erreur de syntaxe : opérateur de redirection sans fichier\n",59);
            return -1;
        }

        // On stocke le nom du fichier de la redirection
        redirections[redir_count].file = strdup(command[i + 1]);
        if (redirections[redir_count].file == NULL) {
            perror("strdup");
            return -1;
        }
        redir_count++;

        // On libère les chaînes de l'opérateur et du fichier
        free(command[i]);
        free(command[i + 1]);

        // et on décale les éléments du tableau command pour supprimer l'opérateur et le fichier
        int j = i;
        while (command[j + 2] != NULL) {
            command[j] = command[j + 2];
            j++;
        }
        command[j] = NULL;
        command[j + 1] = NULL;
    }
    return redir_count;
}

// Sauvegarde les descripteurs de fichiers pour garder leur états initiaux
int save_fds(int fds[3]) {
    for (int i = 0; i < 3; i++) {
        // stdin(0), stdout(1) et stderr(2)
        fds[i] = dup(i);  // Dupliquer le descripteur courant
        if (fds[i] < 0) {
            perror("Erreur de duplication des descripteurs");
            // Si une erreur survient, On annule les duplications précédentes
            for (int j = 0; j < i; j++) close(fds[j]);
            return -1;
        }
    }
    return 0;  // Succès
}

// On remet les descripteurs à leur états initiale pour continuer l'exécution après avoir appliquer la redirection
int restore_fds(int fds[3]) {
    for (int i = 0; i < 3; i++) {
        if (fds[i] != -1) {
            if (dup2(fds[i], i) == -1) {  // On restaure le descripteur original
                perror("Erreur de  restauration des descripteurs");
                return -1;
            }
            close(fds[i]);  // On ferme le descripteur sauvegardé
        }
    }
    return 0;  // Succès
}


// Applique une redirection spécifique en ouvrant le fichier et en redirigeant le descripteur standard
bool apply_redirection(Redirection redir) {
    int fd; // Descripteur de fichier
    int target_fd; // Descripteur cible pour la redirection

    // Définir les flags pour ouvrir le fichier selon le type de redirection
    switch (redir.type) {
        case STDIN:
            fd = open(redir.file, O_RDONLY);  // Fichier en lecture
            target_fd = STDIN_FILENO;
            break;
        case STDOUT:
            fd = open(redir.file, O_WRONLY | O_CREAT | O_EXCL, 0664);  // Écriture sans écraser
            target_fd = STDOUT_FILENO;
            break;
        case STDOUT_APPEND:
            fd = open(redir.file, O_WRONLY | O_CREAT | O_APPEND, 0664);  //Écriture en ajout
            target_fd = STDOUT_FILENO;
            break;
        case STDOUT_TRUNC:
            fd = open(redir.file, O_WRONLY | O_CREAT | O_TRUNC, 0664);  //Écriture avec troncature
            target_fd = STDOUT_FILENO;
            break;
        case STDERR:
            fd = open(redir.file, O_WRONLY | O_CREAT | O_EXCL, 0664);  //Écriture sans écraser
            target_fd = STDERR_FILENO;
            break;
        case STDERR_APPEND:
            fd = open(redir.file, O_WRONLY | O_CREAT | O_APPEND, 0664);  //Écriture en ajout
            target_fd = STDERR_FILENO;
            break;
        case STDERR_TRUNC:
            fd = open(redir.file, O_WRONLY | O_CREAT | O_TRUNC, 0664);  //Écriture avec troncature
            target_fd = STDERR_FILENO;
            break;
        default:
            write(STDERR_FILENO, "Type de redirection inconnu\n",29);
            return false;
    }

    if (fd == -1) {
        perror("Erreur d'ouverture du fichier pour la redirection");
        return false;
    }

    // On redirige le descripteur standard
    // target_fd -> fd
    if (dup2(fd, target_fd) == -1) {
        perror("Erreur lors de dup2 pour redirection");
        close(fd);
        return false;
    }
    close(fd);

    // On ferme le descripteur ouvert car il est maintenant redirigé
    close(fd);
    return true;
}