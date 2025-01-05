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
#include "signal_handler.h"


int main() {
    
    // Configuration des gestionnaires de signaux dans fsh
    setup_signal_handlers();

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
            add_history(line); // On ajoute la commande à l'historique
            char** command = parse_input(line); // parser la commande c'est à dire découper la commande en tokens
            char*** commands = cutout_commands(command); // découper les commandes en tableau de commandes
            
            last_status = handle_commands(commands); // exécuter les commandes une par une 

            //libérer la mémoire allouée pour les commandes
            for (int i = 0; command[i] != NULL; i++) {
                free(command[i]);
            }
            free(command);

        
            for (int i = 0; commands[i] != NULL; i++) {
                for (int j = 0; commands[i][j] != NULL; j++) { 
                    free(commands[i][j]);//On libère  la mémoire allouée pour chaque argument
                }
                free(commands[i]);// On libère la mémoire allouée  pour chaque commande
            }
            free(commands); //On libère  la mémoire allouée pour le tableau de commandes
        }

        free(line); // On libère la mémoire allouée par readline
    }

    return EXIT_SUCCESS;
}