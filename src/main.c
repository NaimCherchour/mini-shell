#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <assert.h>
#include <linux/limits.h> // Pour #define PATH_MAX 4096
#include <signal.h>

// headers
#include "internals.h"
#include "prompt.h"
#include "utils.h"
#include "handler.h"


int main() {
    // clear the screen
    write(STDOUT_FILENO, "\033[H\033[J", 6);


    int status[] = {70, 111, 114, 115, 104, 101, 108, 108, 32, 91, 102, 115, 104, 93, 194, 169, 32, 118, 48, 46, 48, 46, 49, 13};
    for (size_t i = 0; i < sizeof(status) / sizeof(status[0]); i++) {
        write(STDOUT_FILENO, &status[i], 1);
    }
    write(STDOUT_FILENO, "\n", 1);

    // Afficher la date et l'heure actuelles
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

    write(STDOUT_FILENO, "\n", 1);

    // Boucle principale pour lire les commandes utilisateur
    while (1) {
        // Générer le prompt
        rl_outstream = stderr; // Affichage du prompt sur sa sortie d'erreur
        char* prompt = generate_prompt(); // Retourne un prompt alloué dynamiquement
        if (!prompt) {
            write(STDERR_FILENO, "Erreur: échec de la génération du prompt.\n", 43);            
            exit(EXIT_FAILURE);
        }
        // Lire la commande
        char* line = readline(prompt); // line contient la commande entrée par l'utilisateur
        free(prompt); // On libère la mémoire allouée pour le prompt

        if (line == NULL) {
            // Si line est NULL (EOF ou erreur), On quitte la boucle
            exit_shell(NULL);  // exit avec last_status 
        }

        if (strlen(line) > 0) {
    add_history(line);
    char **parsed_line = parse_input(line);
    char ***commands = cutout_commands(parsed_line);

    last_status = handle_commands(commands);

    free_commands(commands);
    for (int i = 0; parsed_line[i] != NULL; i++) free(parsed_line[i]);
    free(parsed_line);
}

        free(line); // On libère la mémoire allouée par readline
    }

    return EXIT_SUCCESS;
}