#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h> // pour PATH_MAX ; 4096 caractères
#include "../include/commandes.h"

char** parse_prompt(char* prompt) {
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

void handle_command(char** command) {
    if ( strcmp (command[0],"cd") == 0 ){ // cd commande interne 
        cd(command);
        return;
    } else if ( strcmp ( command[0],"ftype") == 0 ){ // ftype commande interne
        ftype(command);
        return;
    } else if (strcmp(command[0], "pwd") == 0) { // commande interne pwd
        pwd();  // Appelle la fonction pwd
        return;
    }


    pid_t pid = fork();
    if (pid == 0) {
        // Child process

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
        // printf("`%s` is not recognized as an internal or external command. %s\n", command[0], strerror(errno)); 
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    } else {
        // Fork failed
        perror("fork");
    }
}

int main() {
    //clear the screen
    printf("\033[H\033[J");


    int status[] = {70, 111, 114, 115, 104, 101, 108, 108, 32, 91, 102, 115, 104, 93, 194, 169, 32, 118, 48, 46, 48, 46, 49, 13};
    char* prompt;
    char cwd[1024];
    

    for (size_t i = 0; i < sizeof(status) / sizeof(status[0]); i++) {
        printf("%c", status[i]);
    }
    printf("\n");

    
    // print the date and time
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        execlp("date", "date", NULL);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    } else {
        // Fork failed
        perror("fork");
    }

    printf("\n");



    // reading the input
    while (1) {
        // Get the current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // Create the prompt string
            char prompt_str[1064];
            snprintf(prompt_str, sizeof(prompt_str), "[fsh]%s$ ", cwd);
            rl_outstream = stderr; // L'affichage du prompt de fsh est réalisé sur sa sortie erreur
            prompt = readline(prompt_str);
        } else {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        if (prompt == NULL) {
            break;
        }
   
        if (strlen(prompt) > 0) {
            // Add the input to the history list
            add_history(prompt);
            // Parse the prompt
            char** command = parse_prompt(prompt);
            // Handle the command
            handle_command(command);
            free(command);
        }

        free(prompt);
    }

    return EXIT_SUCCESS;
}