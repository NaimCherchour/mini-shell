#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
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
    // Execute external command
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(command[0], command);
        perror("execvp"); // If execvp returns, there was an error
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
    char* prompt;

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