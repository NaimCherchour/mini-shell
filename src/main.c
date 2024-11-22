#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include <linux/limits.h> // Pour #define PATH_MAX 4096
#include <signal.h>

// headers
#include "internals.h"
#include "prompt.h"


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

int for_loop(char** command);

void handle_command(char** command) {

    //Ignorer SIGINT (Ctrl+C) et SIGTERM
    signal(SIGINT, SIG_IGN); // Ignorer Ctrl+C
    signal(SIGTERM, SIG_IGN); // Ignorer SIGTERM

    // Commandes Internes
    if ( strcmp (command[0],"cd") == 0 ){ // cd
        last_status = cd(command);
        return;
    } else if ( strcmp ( command[0],"ftype") == 0 ){ // ftype 
        last_status = ftype(command);
        return;
    } else if (strcmp(command[0], "pwd") == 0) { // pwd
        last_status = pwd();
        return;
    } else if (strcmp(command[0], "exit") == 0) {
        last_status = exit_shell(command);  // Appelle la fonction exit_shell
        return;
    } else if (strcmp(command[0], "for") == 0){
            for_loop(command);
            return;   
    }


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
        // printf("`%s` is not recognized as an internal or external command. %s\n", command[0], strerror(errno)); 
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
                int wstatus;
        waitpid(pid, &wstatus, 0);  // Attendre la fin du processus enfant
        if (WIFEXITED(wstatus)) {
            // Si le processus enfant s'est terminé normalement
            last_status = WEXITSTATUS(wstatus); // Valeur de retour du programme exécuté
        } else if (WIFSIGNALED(wstatus)) {
            // Si le processus enfant a été tué par un signal
            last_status = 255;  // TODO : 255 ou bien code de retour du signal (128 + numéro du signal) ?
        }
    } else {
        // Fork failed
        perror("fork");
    }
}

int for_loop(char** command){
    // assert(command[0] == "for");
    // assert(command[1] != NULL);
    // assert(command[3] != "in");
    // assert(command[4] != NULL);
    // assert(command[5] != "{");
    // assert(command[6] != NULL);
    // assert(command[7] != "}");

    char var = command[1][0]; // variable (only one letter)
    // debug
    // printf("var: %c\n", var);
    char* directory = command[3]; 
    // debug
    // printf("directory: %s\n", directory);
    char* cmd = command[5];
    // debug
    // printf("cmd: %s\n", cmd);
    
    struct dirent *entry;
    DIR *dp = opendir(directory);

    if (dp == NULL) {
        perror("opendir");
        return errno;
    }

    while ((entry = readdir(dp)) != NULL) {
        // Skip hidden files and directories (those starting with '.')
        if (entry->d_name[0] == '.') {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        // Get file information
        struct stat file_stat;
        if (stat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        // Check if it's a regular file
        if (S_ISREG(file_stat.st_mode)) {
            // construct new char array to store the command and the file
            char* new_command[3]; // command + var or optional arg + NULL
            new_command[0] = cmd;
            if ( '$' == command[6][0]){
                if (command[6][1] == var){
                    new_command[1] = entry->d_name;
                } else {
                    new_command[1] = NULL;
                }
            } else {
                new_command[1] = command[6];
            }
            new_command[2] = NULL;


            // Handle the command
            handle_command(new_command);
        }
    
    }


    closedir(dp);
    return 0;
    
}

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
            char** command = parse_prompt(line); // On découpe la commande en arguments
            if (command) {
                handle_command(command);
                free(command); // On libère la mémoire allouée pour les arguments
            }
        }

        free(line); // On libère la mémoire allouée par readline
    }

    return EXIT_SUCCESS;
}