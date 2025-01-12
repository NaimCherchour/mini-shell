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
#include "../headers/utils.h" // pour for_loop / if_else
#include "../headers/redirections.h" // pour les redirections
#include "../headers/signal_handler.h" // pour les signaux

#define MAX_COMMANDS 10
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

        while (*ptr != '\0' && !isspace(*ptr)) {
            ptr++;
            arg_len++;
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

/**
 * Découpe les commandes successives séparées par le caractère `;`.
 * Chaque commande est stockée dans un tableau de chaînes de caractères.
 * Utilisé pour exécuter des commandes multiples.
 */
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

//Des méthodes pour factoriser le code 

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
        reset_signals_in_child();

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
            if (WTERMSIG(wstatus) == SIGINT) {
                sigint_received = 1;
            }
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

    // On Initialise un tableau de redirections
    //TODO : ne pas restreindre le nombre de redirections
    Redirection redirections[10];
    int redir_count = detect_redirections(command, redirections, 10);
    if (redir_count == -1) {
        return 1; // Erreur de syntaxe : sans fichier 
        // Question : les erreurs de synatxe d'habitude c'est 2 mais sur le sujet en cas d'échec d'une redirection c'est 1 

    }

    // Plus de robustesse :  vérification de la commande à exécuter après les redirections
    if (command[0] == NULL) {
        write(STDERR_FILENO, "Erreur de syntaxe : aucune commande à exécuter\n", 49);
        return 2; // Code de retour pour erreurs de syntaxe
    }

    // On sauvegarde les descripteurs d'origine
    int saved_fds[3];
    if (redir_count > 0 && save_file_descriptors(saved_fds) != 0) {
        write(STDERR_FILENO, "Erreur lors de la sauvegarde des descripteurs\n", 45);
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
// et le if_else , for et les commandes avec des pipes
int handle_commands(char*** commands) {
    sigint_received = 0; // Réinitialiser le flag SIGINT
    int status = 0;
    int i = 0;

    while (commands[i] != NULL) {
        char** cmd = commands[i];

        // Reconstruire la ligne complète pour vérifier les pipes
        char line[4096] = {0};
        for (int j = 0; cmd[j] != NULL; j++) {
            strcat(line, cmd[j]);
            strcat(line, " ");
        }
        if (strlen(line) > 0) {
            line[strlen(line) - 1] = '\0'; // Retirer l'espace final
        }


        if (cmd[0] != NULL && strcmp(cmd[0], "if") == 0) {
            status = if_else(cmd);
        } else if (cmd[0] != NULL && strcmp(cmd[0], "for") == 0) {
            status = for_loop(cmd);
        } else if (strstr(line, " | ")) {  // Détection des pipes
            status = handle_pipes(line);
        } 
        else {
            // Commande simple
            status = execute_command(cmd);
        }
        if (sigint_received){
            // On a reçu un signal SIGINT
            return -1 ; // -1 pour signaler l'interruption et afficher [SIG] dans le prompt
        }
        i++;
    }
    return status;
}

// Fonction auxiliaire 
void run_subcommand(char **args, int index , int total_cmds) {

    // Détection des redirections
    Redirection redirections[10];
    int redir_count = detect_redirections(args, redirections, 10);

    if (redir_count == -1) {
        exit(1);
    }

    // Les contraintes sur les redirections
    char msg[70];
    for (int r = 0; r < redir_count; r++) {
        if ((redirections[r].type == STDIN) && (index != 0)) {
            sprintf(msg, "Erreur : redirection d’entrée impossible sur CMD%d\n", index+1);
            write(STDERR_FILENO, msg, strlen(msg));
            exit(1);
        }
        if ((redirections[r].type == STDOUT || redirections[r].type == STDOUT_APPEND || redirections[r].type == STDOUT_TRUNC) 
            && (index != total_cmds - 1)) {
            sprintf(msg, "Erreur : redirection de sortie impossible sur CMD%d\n", index+1);
            write(STDERR_FILENO,msg,strlen(msg));
            exit(1);
        }
        // REDIR stderr autorisé
    }

    // On sauvegarde les descripteurs d'origine
    int saved_fds[3];
    if (redir_count > 0 && save_file_descriptors(saved_fds) != 0) {
        write(STDERR_FILENO, "Erreur lors de la sauvegarde des descripteurs\n", 45);
        exit(1);
    }

    // On Applique les redirections
    if (redir_count > 0 && !apply_redirections(redirections, redir_count)) {
        restore_file_descriptors(saved_fds);
        exit(1);
    }

    //On exécute interne ou externe
    int ret = execute_internal_command(args, 0, NULL);
    if (ret == -1) {
        ret = execute_external_command(args);
    }
    // On restaure les descripteurs de fichiers après l'exécution
    if (redir_count > 0) {
        restore_file_descriptors(saved_fds);
    }
    exit(ret);
}


// handle_pipes appelle run_subcommand pour chaque sous-commande du pipeline
int handle_pipes(char *line) {
    char *commands[MAX_COMMANDS]; // Tableau pour stocker les sous-commandes
    int num_commands = 0;

    // Découper la ligne en sous-commandes autour de '|'
    char *token = strtok(line, "|");
    while (token && num_commands < MAX_COMMANDS) {
        while (*token == ' ') token++; // Supprimer les espaces au début
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0'; // Supprimer les espaces à la fin
        commands[num_commands++] = strdup(token);
        if (commands[num_commands - 1] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, "|");

    }
    commands[num_commands] = NULL; // Terminaison

    int pipefds[2 * (num_commands - 1)]; // Tableau pour stocker les pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefds + 2 * i) == -1) {
            perror("pipe");
            return 1;
        }
    }

    pid_t pids[num_commands]; // PIDs des processus enfants
    int status = 0;

    // Exécuter chaque sous-commande
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        if (pids[i] == 0) { // Processus enfant

            // Redirection entrée
            if (i > 0) {
                dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
            }
            // Redirection sortie
            if (i < num_commands - 1) {
                dup2(pipefds[2 * i + 1], STDOUT_FILENO);
            }
            // Fermer tous les pipes inutiles
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipefds[j]);
            }
            
            // Exécuter la commande
            char **args = parse_input(commands[i]);
            run_subcommand(args,i,num_commands);
        } else if (pids[i] < 0) {
            perror("fork");
            status = 1;
        }
    }

    // Fermer tous les pipes dans le parent
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipefds[i]);
    }

    // Attendre tous les enfants
    for (int i = 0; i < num_commands; i++) {
        waitpid(pids[i], &status, 0);
    }

    // Libérer la mémoire
    for (int i = 0; i < num_commands; i++) {
        free(commands[i]);
    }

    // On Normalise le statut de retour
    if (WIFEXITED(status)) {
        status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        status = 128 + WTERMSIG(status);
    }

    return status;
}