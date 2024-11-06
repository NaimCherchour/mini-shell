#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

int main() {
    char* prompt;

    while ((prompt = readline("[fsh]$ ")) != NULL) {
        if (strcmp(prompt, "exit") == 0) {
            free(prompt);
            break;
        }

        // Add the input to the history list
        if (strlen(prompt) > 0) {
            add_history(prompt);
            printf("%s\n", prompt);
        }

        free(prompt);
    }

    return 0;
}