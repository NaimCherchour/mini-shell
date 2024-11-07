#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

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

    int status[] = {102, 115, 104, 101, 108, 108, 32, 91, 102, 115, 104, 93, 36, 194, 169, 32, 118, 48, 46, 48, 46, 49, 32, 119, 114, 105, 116, 116, 101, 110, 32, 98, 121, 32, 89, 97, 99, 105, 110, 101, 46, 32, 87, 101, 108, 99, 111, 109, 101, 33};
    

    
    char* prompt;
    

    for (int i = 0; i < sizeof(status) / sizeof(status[0]); i++) {
        printf("%c", status[i]);
    }
    printf("\n");
    while ((prompt = readline("[fsh]$ ")) != NULL) {
        // Exit the shell
        if (strcmp(prompt, "fin") == 0) {
            free(prompt);
            break;
        }

        // Add the input to the history list
        if (strlen(prompt) > 0) {
            add_history(prompt);
            char** command = parse_prompt(prompt);
            handle_command(command);
            free(command);
        }

        free(prompt);
    }

    return 0;
}