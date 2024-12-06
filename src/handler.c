#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <linux/limits.h> // Pour #define PATH_MAX 4096
#include <signal.h>

#include "../headers/handler.h"
#include "../headers/prompt.h" // pour utiliser la variable last_status ie valeur de retour 
#include "../headers/internals.h" // pour les commandes internes
#include "../headers/utils.h" // pour for_loop 


// Lire la ligne de commande
char** parse_input(char* prompt) {
    // Allocate memory for arguments
    char** args = malloc(10 * sizeof(char*)); // max 10 arguments
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Separate the command and the arguments
    char* arg = strtok(prompt, " ");
    int arg_count = 0;

    while (arg != NULL) {
        args[arg_count] = arg;
        arg_count++;

        if (arg_count >= 10) {
            // realloc the arguments array
            char** temp = realloc(args, 2 * arg_count * sizeof(char*));
            if (temp == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }

            args = temp;

            // Set the rest of the array to NULL
            for (int i = arg_count; i < 2 * arg_count; i++) {
                args[i] = NULL;
            }

        }

        arg = strtok(NULL, " ");
    }

    // Null-terminate the arguments array
    args[arg_count] = NULL;

    return args;
}



int handle_command(char** command) {

    //Ignorer SIGINT (Ctrl+C) et SIGTERM
    signal(SIGINT, SIG_IGN); // Ignorer Ctrl+C
    signal(SIGTERM, SIG_IGN); // Ignorer SIGTERM

    // Commandes Internes
    if ( strcmp (command[0],"cd") == 0 ) return cd(command);
    if ( strcmp ( command[0],"ftype") == 0 ) return ftype(command);
    if (strcmp(command[0], "pwd") == 0) return pwd();
    if (strcmp(command[0], "exit") == 0) return exit_shell(command);  // Appelle la fonction exit_shell
    if (strcmp(command[0], "for") == 0) return for_loop(command);
    if (strcmp(command[0], "if") == 0) return if_else(command);

    

    pid_t pid = fork();
    if (pid == 0) {
        // Child process

        // On rétablit les signaux à leurs comportements par défaut donc si un signal ( ex SIGINT) est reçu le processus sera tué
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL); 

        // Check if the command is a built-in
        char* bin_path = "bin/";
        // Allocate memory for the command path string
        char* command_path = malloc(strlen(bin_path) + strlen(command[0]) + 1);
        if (command_path == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        // Copy the bin path and the command name to the command path string
        strcpy(command_path, bin_path);
        strcat(command_path, command[0]);


        // Check if the command is a built-in
        if (access(command_path, X_OK) == 0) {
            // Execute built-in command
            if (execv(command_path, command) == -1) {
                perror("execv");
                exit(EXIT_FAILURE);
            }
        }
        free(command_path);

        // Execute external command
        execvp(command[0], command);

        // If execvp fails then there is no such command
        perror(command[0]);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
                int wstatus;
        waitpid(pid, &wstatus, 0);  // Attendre la fin du processus enfant
        if (WIFEXITED(wstatus)) {
            // Si le processus enfant s'est terminé normalement
            return WEXITSTATUS(wstatus); // Valeur de retour du programme exécuté
        } else if (WIFSIGNALED(wstatus)) {
            // Si le processus enfant a été tué par un signal
            return 255;  // TODO : 255 ou bien code de retour du signal (128 + numéro du signal) ?
        }
    } else {
        // Fork failed
        perror("fork");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}