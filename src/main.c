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
    printf("\033[H\033[J");


    int status[] = {70, 111, 114, 115, 104, 101, 108, 108, 32, 91, 102, 115, 104, 93, 194, 169, 32, 118, 48, 46, 48, 46, 49, 13};
    for (size_t i = 0; i < sizeof(status) / sizeof(status[0]); i++) {
        printf("%c", status[i]);
    }
    printf("\n");

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

    printf("\n");

    // Boucle principale pour lire les commandes utilisateur
    while (1) {
        // Générer le prompt
        rl_outstream = stderr; // Affichage du prompt sur sa sortie d'erreur
        char* prompt = generate_prompt(); // Retourne un prompt alloué dynamiquement
        if (!prompt) {
            fprintf(stderr, "Erreur: échec de la génération du prompt.\n");
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
            add_history(line); // On ajoute la commande à l'historique
            char** command = parse_command(line); // On découpe la commande en arguments
            if (command) {
                handle_command(command);
                free(command); // On libère la mémoire allouée pour les arguments
            }
        }

        free(line); // On libère la mémoire allouée par readline
    }

    return EXIT_SUCCESS;
}