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

#define MAX_COMMANDS 5
#define MAX_ARGS 10
#define INITIAL_SIZE 15

char** parse_input(char* prompt) {
    int size = INITIAL_SIZE;
    char** args = malloc(size * sizeof(char*));
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int arg_count = 0;
    char* token = strtok(prompt, " ");

    while (token != NULL) {
        if (arg_count >= size - 1) {
            size *= 2;
            char** temp = realloc(args, size * sizeof(char*));
            if (temp == NULL) {
                perror("realloc");
                free(args);
                exit(EXIT_FAILURE);
            }
            args = temp;
        }

        args[arg_count] = strdup(token);
        if (args[arg_count] == NULL) {
            perror("strdup");
            for (int i = 0; i < arg_count; i++) {
                free(args[i]);
            }
            free(args);
            exit(EXIT_FAILURE);
        }

        arg_count++;
        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL;
    return args;
}




// cuts-out commands from the parsed prompt
char*** cutout_commands(char** parsed_prompt) {
    // Allocate memory for the commands 2D array
    char*** commands = (char***)malloc(MAX_COMMANDS * sizeof(char**));
    if (commands == NULL) {
        perror("Failed to allocate memory for commands");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_COMMANDS; i++) {
        commands[i] = (char**)malloc(MAX_ARGS * sizeof(char*));
        if (commands[i] == NULL) {
            perror("Failed to allocate memory for commands[i]");
            exit(EXIT_FAILURE);
        }
    }

    // Loop through the parsed prompt
    int i = 0;
    int j = 0;
    int k = 0;
    while (parsed_prompt[i] != NULL) {
        // Check for the ; separator
        if (strcmp(parsed_prompt[i], ";") == 0) {
            commands[j][k] = NULL;
            j++;
            k = 0;
        } else {
            commands[j][k] = parsed_prompt[i];
            k++;
        }
        i++;
    }
    commands[j][k] = NULL; // Ensure the last command is null-terminated
    commands[j+1][0] = NULL; // Null-terminate the commands array


    return commands;
}

void free_commands(char*** commands) {
    for (int i = 0; i < MAX_COMMANDS; i++) {
        free(commands[i]);
    }
    free(commands);
}




// executes one simple command
int execute_command(char** command) {

    //Ignorer SIGINT (Ctrl+C) et SIGTERM
    signal(SIGINT, SIG_IGN); // Ignorer Ctrl+C
    signal(SIGTERM, SIG_IGN); // Ignorer SIGTERM

    // Commandes Internes
    if (strcmp(command[0],"cd") == 0) return cd(command);
    if (strcmp(command[0],"ftype") == 0) return ftype(command);
    if (strcmp(command[0], "pwd") == 0) {
        //Si des arguments supplémentaires sont données à `pwd`
        if (command[1] != NULL) {
            write(STDERR_FILENO, "pwd: too many arguments\n", 24);
            return 1;
        }
        return pwd();
    }
    if (strcmp(command[0], "exit") == 0) return exit_shell(command);
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
            return -WIFSIGNALED(wstatus); // valeur < 0 pour détecter les SIG et exit nous retourne bien 255 en faisant echo $?
        }
    } else {
        // Fork failed
        perror("fork");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// executes the commands one by one
// TODO: detect piping and redirections
int handle_commands(char*** commands) {
    int i = 0;
    while (commands[i][0] != NULL) {
        //debug
        // printf("command: %s\n", commands[i][0]);
        int status = execute_command(commands[i]);
        if (status != 0) return status;
        i++;
    }
    return EXIT_SUCCESS;
}