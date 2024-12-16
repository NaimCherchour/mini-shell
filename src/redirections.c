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
#include <fcntl.h>


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
            write(STDERR_FILENO, "Erreur de syntaxe : opérateur de redirection sans fichier\n",54);
            return -1;
        }

        // On stocke le nom du fichier de la redirection
        redirections[redir_count].file = strdup(command[i + 1]);
        if (redirections[redir_count].file == NULL) {
            perror("strdup");
            // free ?
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




