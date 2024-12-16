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
#include <ctype.h> // Pour la fonction isspace

#include "../headers/handler.h"
#include "../headers/prompt.h" // pour utiliser la variable last_status ie valeur de retour 
#include "../headers/internals.h" // pour les commandes internes
#include "../headers/utils.h" // pour for_loop 
#include "../headers/redirections.h" // pour les redirections

#define MAX_COMMANDS 5
#define MAX_ARGS 10
#define INITIAL_SIZE 15


// Parse l'entrée de l'utilisateur ie: découpe la ligne en arguments
char** parse_input(char* line) {
    int size = INITIAL_SIZE;
    char** args = malloc(size * sizeof(char*));
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int arg_count = 0;
    char* ptr = line ; 

    while (*ptr != '\0') {
        // Ignorer les espaces
        while (isspace(*ptr)) ptr++;

        // Vérifier si on a atteint la fin de la ligne
        if (*ptr == '\0') break;
        
        char* arg_start = ptr; // pointeur sur le début de l'argument
        int arg_len = 0; // longueur de l'argument

        if (*ptr == '{' || *ptr == '}' || *ptr == ';') {
            // Accolades et point-virgule sont des tokens uniques
            arg_len = 1;
            ptr++;
        } else {
            while (*ptr != '\0' && !isspace(*ptr) && *ptr != '{' && *ptr != '}' && *ptr != ';') {
                ptr++;
                arg_len++;
            }
        }

        char* arg = strndup(arg_start, arg_len); // copie de l'argument
        args[arg_count++] = arg; // on ajoute l'argument au tableau
 
        if (arg_count >= size ) { // on vérifie si on a atteint la taille maximale
            size *= 2; 
            args = realloc(args, size * sizeof(char*)); 
            if (args == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
    }
    args[arg_count] = NULL; // on termine le tableau avec NULL
    return args;
}

// cutout_commands découpe les commandes séparées par des ;
char*** cutout_commands(char** args) {
    int size = INITIAL_SIZE;
    // Allocate memory for the commands 2D array
    char*** commands = malloc(size * sizeof(char**));
    if (!commands) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    int cmd_count = 0;
    int arg_index = 0;

    while (args[arg_index] != NULL ){
        int cmd_size = INITIAL_SIZE;
        char** cmd_args = malloc(cmd_size * sizeof(char*));
        if (!cmd_args) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        int cmd_arg_count = 0;
        int in_brackets = 0; // Pour gérer les accolades
        
        while(args[arg_index] != NULL ) {
            if (strcmp(args[arg_index],"{") == 0 ) {
                in_brackets++;
            } else if (strcmp(args[arg_index],"}") == 0 ) {
                in_brackets--; 
            }

            if (strcmp(args[arg_index],";") == 0 && in_brackets <= 0 ) {
                arg_index++; //On saute le ppoint-virgule
                break ; 
            }
            cmd_args[cmd_arg_count++] = strdup(args[arg_index++]);

            if (cmd_arg_count >= cmd_size) {
                cmd_size *= 2;
                cmd_args = realloc(cmd_args, cmd_size * sizeof(char*));
                if (!cmd_args) {
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
            }
        } 

        cmd_args[cmd_arg_count] = NULL; //Null-terminate la commande
        commands[cmd_count++] = cmd_args;

        if (cmd_count >= size) {
            size *= 2;
            commands = realloc(commands, size * sizeof(char**));
            if (!commands) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    commands[cmd_count] = NULL; // Null-terminate the commands array
    return commands;
}

void free_commands(char*** commands) {
    if (commands == NULL) {
        return; // Nothing to free
    }
    for (int i = 0; i < MAX_COMMANDS; i++) {
        if ( commands[i] != NULL ){
                free(commands[i]);
        }
    }
    free(commands);
}


//Des méthodes pour factoriser le code 

// Gérer les signaux
void ignore_signals() {
    signal(SIGINT, SIG_IGN); // Ignorer Ctrl+C
    signal(SIGTERM, SIG_IGN); // Ignorer SIGTERM
}

// Sauvegarder les descripteurs d'origine
int save_file_descriptors(int* saved_fds) {
    return save_fds(saved_fds);
}

// Appliquer les redirections
int apply_redirections(Redirection* redirections, int redir_count) {
    for (int i = 0; i < redir_count; i++) {
        if (!apply_redirection(redirections[i])) {
            free(redirections[i].file);
            return 0;
        }
        free(redirections[i].file);
    }
    return 1;
}

// Restaurer les descripteurs de fichiers
void restore_file_descriptors(int* saved_fds) {
    restore_fds(saved_fds);
}

// Exécuter une commande interne 
int execute_internal_command(char** command, int redir_count, int* saved_fds) {
    int res ; // la valeur à retourner 

    // Commandes Internes
    if (strcmp(command[0], "cd") == 0) {
        res = cd(command);
    } else if (strcmp(command[0], "ftype") == 0) {
        res = ftype(command);
    } else if (strcmp(command[0], "pwd") == 0) {
        res = pwd(command);
    } else if (strcmp(command[0], "exit") == 0) {
        res = exit_shell(command);
    } else if (strcmp(command[0], "for") == 0) {
        res =  for_loop(command);
    } else if (strcmp(command[0], "if") == 0) {
        res =  if_else(command);
    } else {
        return -1; // Commande non interne
    }

    if (redir_count > 0) {
        restore_file_descriptors(saved_fds); // on remets les descripteurs à leur état initial
    }
    return res;
}

// Exécuter une commande externe
int execute_external_command(char** command) {
    pid_t pid = fork();
    if (pid == 0) { // Enfant
        // Réinitialiser les signaux
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        // Créer le chemin d'accès à la commande
        char* bin_path = "bin/";
        char* command_path = malloc(strlen(bin_path) + strlen(command[0]) + 1);
        if (command_path == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(command_path, bin_path);
        strcat(command_path, command[0]);

        // Vérifier si la commande est exécutable
        if (access(command_path, X_OK) == 0) {
            if (execv(command_path, command) == -1) {
                perror("execv");
                exit(EXIT_FAILURE);
            }
        }
        free(command_path);

        // Exécuter la commande externe
        execvp(command[0], command);
        perror(command[0]);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent
        int wstatus;
        waitpid(pid, &wstatus, 0); // Attendre le processus enfant

        if (WIFEXITED(wstatus)) {
            return WEXITSTATUS(wstatus);
        } else if (WIFSIGNALED(wstatus)) {
            return -WIFSIGNALED(wstatus);
        }
    } else {
        perror("fork");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// executes one simple command
int execute_command(char** command) {

    // On Ignore les signaux : SIGINT (Ctrl+C) et SIGTERM
    ignore_signals();

    // On Initialise un tableau de redirections
    //TODO : ne pas restreindre le nombre de redirections
    Redirection redirections[5];
    int redir_count = detect_redirections(command, redirections, 5);

    // On sauvegarde les descripteurs d'origine
    int saved_fds[3];
    if (redir_count > 0 && save_file_descriptors(saved_fds) != 0) {
        fprintf(stderr, "Erreur lors de la sauvegarde des descripteurs\n");
        return EXIT_FAILURE;
    }

    // On Applique les redirections
    if (redir_count > 0 && !apply_redirections(redirections, redir_count)) {
        restore_file_descriptors(saved_fds);
        return EXIT_FAILURE;
    }

    // On Exécute la commande interne ou externe
    int res = execute_internal_command(command,redir_count, saved_fds);
    if (res != -1) {
        return res;
    }
    res = execute_external_command(command);


    // On restaure les descripteurs de fichiers après l'exécution
    if (redir_count > 0) {
        restore_file_descriptors(saved_fds);
    }

    return res;
}

// Exécute les commandes une par une ( pour les séquences de commandes , séparées par des ;)
int handle_commands(char*** commands) {
    int status = 0;
    int i = 0;
    while (commands[i] != NULL) {
        status = execute_command(commands[i]);
        i++;
    }
    return status; // On ret la valeur de retour de la dernière commande exécutée
}