#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include <linux/limits.h> // Pour #define PATH_MAX 4096
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>


#include "../headers/utils.h"
#include "../headers/handler.h" // pour handle_commands
#include "../headers/prompt.h" // pour last_status
#include "../headers/signal_handler.h" // pour sigint_received
#include <fcntl.h>

// Macros
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_DEPTH 100


int for_syntaxe(char **command) {
    // Vérification de la syntaxe de base du 'for'
    int argc = 0;
    while (command[argc] != NULL) {
        argc++;
    }

    if (argc < 4) {
        write(STDERR_FILENO, "Usage: for f in dir { cmd $f }\n", 31);
        return 2;
    }

    if (strcmp(command[2], "in") != 0) {
        write(STDERR_FILENO, "Erreur : mot-clé 'in' manquant après la variable.\n", 50);
        return 2;
    }

    // Vérification de la présence de '{' et '}'
    int has_open_bracket = 0, has_close_bracket = 0;
    for (int i = 0; command[i] != NULL; i++) {
        if (strcmp(command[i], "{") == 0) {
            has_open_bracket = 1;
        }
        if (strcmp(command[i], "}") == 0) {
            has_close_bracket = 1;
        }
    }

    if (!has_open_bracket || !has_close_bracket) {
        write(STDERR_FILENO, "Erreur : accolades manquantes pour délimiter la commande\n", 58);
        return 2;
    }

    return 0;
}

// Parcoure un répertoire et exécute des commandes sur chaque fichier/s.rep
/*
    * directory : le répertoire à parcourir
    * cmd_str : la commande à exécuter
    * hidden : 1 pour inclure les fichiers cachés, 0 sinon
    * recursive : 1 pour parcourir récursivement les sous-répertoires, 0 sinon
    * extension : 1 pour filtrer par extension, 0 sinon
    * EXT : l'extension à filtrer
    * type : 1 pour filtrer par type (fichier, répertoire, lien symbolique, fifo), 0 sinon
    * TYPE : le type à filtrer
    * var : la variable à remplacer dans la commande
    * return_val : la valeur de retour actuelle
    * depth : la profondeur actuelle de récursion
    * return : la valeur de retour maximale
 */
int browse_directory(const char *directory, char *cmd_str, int hidden, int recursive, int extension, char *EXT, int type, char TYPE, int parallel, int MAX_PROCESSES, char var, int return_val, int depth) {
    // Limit recursion depth
    if (depth > MAX_DEPTH) {
        write(STDERR_FILENO, "browse_directory: Recursion depth too high.\n", 45);
        return 1;
    }

    DIR *dp;
    struct dirent *entry;

    dp = opendir(directory);
    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    if (parallel == 1) {
        pid_t id;
        int active_processes = 0;
        int entries_processed = 0;

        while ((entry = readdir(dp)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Handle hidden files
            if (!hidden && entry->d_name[0] == '.')
                continue;

            // Wait if max processes are running
            if (active_processes >= MAX_PROCESSES) {
                int status;
                wait(&status);
                active_processes--;

                if (WIFEXITED(status)) {
                    return_val = MAX(return_val, WEXITSTATUS(status));
                } else {
                    return_val = 1; // Error occurred
                }
            }

            // Fork a child process to handle this entry
            id = fork();
            if (id == -1) {
                perror("fork");
                return_val = 1;
                continue;
            }

            if (id == 0) {
                // Child process

                // Build the full path
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

                // Get file info
                struct stat file_stat;
                if (lstat(full_path, &file_stat) == -1) {
                    perror("lstat");
                    exit(1);
                }

                // Recurse into subdirectories if needed
                if (recursive && S_ISDIR(file_stat.st_mode)) {
                    exit(browse_directory(full_path, cmd_str, hidden, recursive, extension, EXT, type, TYPE, parallel, MAX_PROCESSES, var, return_val, depth + 1));
                }

                // Check file type if needed
                if (type) {
                    if ((TYPE == 'f' && !S_ISREG(file_stat.st_mode)) ||
                        (TYPE == 'd' && !S_ISDIR(file_stat.st_mode)) ||
                        (TYPE == 'l' && !S_ISLNK(file_stat.st_mode)) ||
                        (TYPE == 'p' && !S_ISFIFO(file_stat.st_mode))) {
                        exit(0); // Skip unsupported types
                    }
                }

                // Handle file extension if required
                char *var_value;
                if (extension) {
                    char *dot = strrchr(entry->d_name, '.');
                    if (!dot || strcmp(dot + 1, EXT) != 0) {
                        exit(0);
                    }
                    size_t name_len = dot - entry->d_name;
                    size_t dir_len = strlen(directory);
                    var_value = malloc(dir_len + 1 + name_len + 1);
                    if (!var_value) {
                        perror("malloc");
                        exit(1);
                    }
                    snprintf(var_value, dir_len + 1 + name_len + 1, "%s/%.*s", directory, (int)name_len, entry->d_name);
                } else {
                    var_value = strdup(full_path);
                    if (!var_value) {
                        perror("strdup");
                        exit(1);
                    }
                }

                // Replace the variable with the appropriate value
                char var_str[3] = {'$', var, '\0'};
                char *replaced_cmd = replace_var_with_path(cmd_str, var_str, var_value);
                free(var_value);
                if (!replaced_cmd) {
                    write(STDERR_FILENO, "Error replacing variable.\n", 26);
                    exit(1);
                }

                // Execute the block
                int result = execute_block(replaced_cmd);
                free(replaced_cmd);
                exit(result);
            } else {
                // Parent process
                active_processes++;
                entries_processed++;
            }
        }

        // Wait for remaining child processes
        while (active_processes > 0) {
            int status;
            wait(&status);
            active_processes--;

            if (WIFEXITED(status)) {
                return_val = MAX(return_val, WEXITSTATUS(status));
            } else {
                return_val = 1;
            }
        }

        if (closedir(dp) == -1) {
            perror("closedir");
            return 1;
        }

        return return_val;
    } else {
            while ((entry = readdir(dp)) != NULL) {
        // Ignorer '.' et '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Gérer les fichiers cachés
        if (!hidden && entry->d_name[0] == '.')
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        // Obtenir les informations du fichier
        struct stat file_stat;
        if (lstat(full_path, &file_stat) == -1) {
            perror("lstat");
            return_val = 1;
            continue;
        }

        // Parcours récursif des sous-répertoires
        if (recursive && S_ISDIR(file_stat.st_mode)) {
            int sub_return_val = browse_directory(full_path, cmd_str, hidden, recursive, extension, EXT, type, TYPE, parallel, MAX_PROCESSES, var, return_val, depth + 1);
            return_val = MAX(return_val, sub_return_val);
        }

        // Vérifier le type si nécessaire
        if (type) {
            if ((TYPE == 'f' && !S_ISREG(file_stat.st_mode)) ||
                (TYPE == 'd' && !S_ISDIR(file_stat.st_mode)) ||
                (TYPE == 'l' && !S_ISLNK(file_stat.st_mode)) ||
                (TYPE == 'p' && !S_ISFIFO(file_stat.st_mode))) {
                continue;
            }
        }

        // Vérifier l'extension si nécessaire
        char *var_value;
        if (extension) {
            char *dot = strrchr(entry->d_name, '.');
            if (!dot || strcmp(dot + 1, EXT) != 0) {
                continue;
            }
            // Amputer l'extension et inclure le chemin
            size_t name_len = dot - entry->d_name;
            size_t dir_len = strlen(directory);
            var_value = malloc(dir_len + 1 + name_len + 1); // +1 pour '/' et +1 pour '\0'
            if (!var_value) {
                perror("malloc");
                return_val = 1;
                continue;
            }
            snprintf(var_value, dir_len + 1 + name_len + 1, "%s/%.*s", directory, (int)name_len, entry->d_name);
        } else {
            var_value = strdup(full_path);
            if (!var_value) {
                perror("strdup");
                return_val = 1;
                continue;
            }
        }

        // Remplacer la variable par la valeur appropriée
        char var_str[3] = {'$', var, '\0'};
        char *replaced_cmd = replace_var_with_path(cmd_str, var_str, var_value);
        free(var_value);
        if (!replaced_cmd) {
            write(STDERR_FILENO, "Erreur lors du remplacement de la variable.\n", 44);
            return_val = 1;
            continue;
        }

    
        // Sequential execution
        int result = execute_block(replaced_cmd);
        if (result == -1) {
            // Received SIGINT during command execution
            return -1;
        }
        return_val = MAX(return_val, result);
        free(replaced_cmd);
        
            
    }

    if (closedir(dp) == -1) {
        perror("closedir");
        return 1;
    }
    return return_val;

    }
}


char* replace_var_with_path(const char* str, const char* var, const char* replacement) {
    char* result;
    char* ins;
    char* tmp;
    size_t len_var;
    size_t len_replacement;
    size_t len_front;
    size_t count;

    if (!str || !var) {
        return NULL;
    }
    len_var = strlen(var);
    if (len_var == 0) {
        return NULL;
    }
    if (!replacement) {
        replacement = "";
    }
    len_replacement = strlen(replacement);

    ins = (char*)str;
    for (count = 0; (tmp = strstr(ins, var)); ++count) {
        ins = tmp + len_var;
    }

    tmp = result = malloc(strlen(str) + (len_replacement - len_var) * count + 1);

    if (!result) {
        return NULL;
    }

    while (count--) {
        ins = strstr(str, var);
        len_front = ins - str;
        memcpy(tmp, str, len_front);
        tmp += len_front;
        memcpy(tmp, replacement, len_replacement);
        tmp += len_replacement;

        str += len_front + len_var;
    }
    strcpy(tmp, str);
    return result;
}

int constructor(char **command, char *var, char **directory, int *hidden, int *recursive, int *extension, char **EXT, int *type, char *TYPE, int *parallel, int *MAX_PROCESSES, char **cmd_str ) {
    // Extraction de la variable et du répertoire
    *var = command[1][0];
    *directory = command[3];

    // Analyse des options
    int i = 4; // Position après 'in' et le répertoire
    while (command[i] != NULL && command[i][0] == '-') {
        if (strcmp(command[i], "-A") == 0) {
            *hidden = 1;
        } else if (strcmp(command[i], "-r") == 0) {
            *recursive = 1;
        } else if (strcmp(command[i], "-e") == 0) {
            if (command[i+1] != NULL) {
                *extension = 1;
                *EXT = command[i+1];
                i++;
            } else {
                write(STDERR_FILENO, "Erreur : option '-e' nécessite un argument.\n", 42);
                return 1;
            }
        } else if (strcmp(command[i], "-t") == 0) {
            if (command[i+1] != NULL && strlen(command[i+1]) == 1) {
                *type = 1;
                *TYPE = command[i+1][0];
                i++;
            } else {
                write(STDERR_FILENO, "Erreur : option '-t' nécessite un argument d'un caractère.\n", 62);
                return 1;
            }
        } else if(strcmp(command[i], "-p") == 0){
            if (command[i+1] != NULL && atoi(command[i+1]) > 0) {
                *parallel = 1;
                *MAX_PROCESSES = atoi(command[i+1]);
                i++;
            } else {
                write(STDERR_FILENO, "Erreur : option -p nécessite un argument d'un entier positif.\n", 62);
                return 1;
            }
        } else {
            char buffer[256];
            int len = snprintf(buffer, sizeof(buffer), "Erreur : option inconnue '%s'\n", command[i]);
            write(STDERR_FILENO, buffer, len);
            return 1;
        }
        i++;
    }

    // Extraction du bloc de commandes entre '{' et '}'
    if (command[i] == NULL || strcmp(command[i], "{") != 0) {
        write(STDERR_FILENO, "Erreur : '{' manquante ou mal positionnée pour ouvrir la commande structurée.\n", 81);
        return 2;
    }

    // On cherche la fin du bloc '{...}
    int bracket_count = 1;
    int cmd_start = i + 1;
    int cmd_end = -1;
    for (int j = i + 1; command[j] != NULL; j++) {
        if (strcmp(command[j], "{") == 0) {
            bracket_count++;
        } else if (strcmp(command[j], "}") == 0) {
            bracket_count--;
            if (bracket_count == 0) {
                cmd_end = j;
                //break;
            }
        }
    }

    if (bracket_count != 0) {
        write(STDERR_FILENO, "Erreur de syntaxe : accolades manquantes ou mal placées.\n", 58);
        return 2;
    }


    // Construire la chaîne de commande
    size_t total_length = 0;
    for (int j = cmd_start; j < cmd_end; j++) {
        total_length += strlen(command[j]) + 1; // +1 pour l'espace
    }

    *cmd_str = malloc(total_length + 1); // +1 pour le '\0'
    if (!*cmd_str) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    (*cmd_str)[0] = '\0';

    for (int j = cmd_start; j < cmd_end; j++) {
        strcat(*cmd_str, command[j]);
        strcat(*cmd_str, " ");
    }

    return 0;
}




// Méthode interne 
int execute_block(char* block_command) {
    // On vérifie si SIGINT a été reçu avant l'exécution
    if (sigint_received){
        sigint_received = 0;
        return -1;
    }
    // On analyse la chaîne avec parse_input
    char **tokens = parse_input(block_command);
    // On divise les tokens en commandes avec split_commands
    char ***commands = cutout_commands(tokens);
    // On exécute les commandes
    int return_val = handle_commands(commands);
    //On Libère la mémoire
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
    for (int i = 0; commands[i] != NULL; i++) {
        for (int j = 0; commands[i][j] != NULL; j++) {
            free(commands[i][j]);
        }
        free(commands[i]);
    }
    free(commands);
    return return_val;
}

/**
 * Cette méthode auxiliaire convertit un tableau de tokens en une chaîne de caractères
 * Chaque token est séparé par un espace dans la chaîne finale
 *
 * @param tokens Le tab de tokens
 * @param count  Le nombre de tokens dans le tab
 * @return       Une chaîne de caractères contenant tous les tokens séparés par des espaces
 */
char* tokens_to_string(char** tokens, int count) {
    size_t total_length = 0;
    // Calcul de la longueur
    for (int i = 0; i < count; i++) {
        total_length += strlen(tokens[i]) + 1; // +1 pour l'espace ou le caractère de fin
    }

    char* result = malloc(total_length + 1); // +1 pour le caractère nul '\0'
    if (!result) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    result[0] = '\0'; // Init
    // On concat chaque token suivi d'un espace à la chaîne résultante
    for (int i = 0; i < count; i++) {
        strcat(result, tokens[i]);
        strcat(result, " ");
    }
    return result;
}


int for_loop(char **command) {
    // Étape 1 : Vérification de la syntaxe du 'for' en utilisant 'for_syntaxe'
    int syntax_check = for_syntaxe(command);
    if (syntax_check != 0) {
        return syntax_check;
    }

    // Étape 2 : Extraction de la variable, le répertoire et les options à l'aide de 'constructor'
    char var;
    char *directory;
    int hidden = 0, recursive = 0, extension = 0, type = 0, parallel = 0;
    char *EXT = NULL;
    char TYPE = '\0';
    int MAX_PROCESSES = 0;
    char *cmd_str = NULL;

    int constructor_result = constructor(command, &var, &directory, &hidden, &recursive, &extension, &EXT, &type, &TYPE, &parallel, &MAX_PROCESSES, &cmd_str);
    if (constructor_result != 0) {
        return constructor_result;
    }


    // Étape 3 : Parcour du répertoire en utilisant 'browse_directory'
    int return_val = 0;
    return_val = browse_directory(directory, cmd_str, hidden, recursive, extension, EXT, type, TYPE, parallel, MAX_PROCESSES, var, return_val, 0);

    // Libération de la mémoire allouée
    free(cmd_str);
    return return_val;
}

// Vérifie si la condition 'TEST' est un pipeline valide
bool is_valid_test_condition(char **test_cmd_tokens, int token_count) {
    if (strcmp(test_cmd_tokens[0], "if") == 0 || strcmp(test_cmd_tokens[0], "for") == 0)   {
        return false;
    } else {
        return true;
    }
}

// if TEST { CMD } else { CMD }
// TEST is a pipeline of commands that return 0 or 1
int if_else(char** command) {
    int i = 1; // 'if' est à command[0]
    int argc = 0;
    while (command[argc] != NULL) {
        argc++;
    }

    // Vérification de la présence de la condition
    if (command[i] == NULL) {
        write(STDERR_FILENO, "Erreur de syntaxe : condition manquante après 'if'\n", 53);
        return 2;
    }

    // Extraction de la condition jusqu'à la première '{'
    int test_start = i;
    while (command[i] != NULL && strcmp(command[i], "{") != 0) {
        i++;
    }

    if (command[i] == NULL) {
        write(STDERR_FILENO, "Erreur de syntaxe : '{' manquant après la condition\n", 54);
        return 2;
    }

    int test_end = i - 1;
    
    //si la condition TEST est vide
    if (test_start > test_end) {
        write(STDERR_FILENO, "Erreur de syntaxe : condition 'TEST' vide\n", 43);
        return 2;
    }

    // Extraction des tokens de la condition
    int test_token_count = test_end - test_start + 1; //Nombre de tokens dans la condition
    char **test_cmd = malloc((test_token_count + 1) * sizeof(char *));
    if (!test_cmd) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int j = test_start, k = 0; j <= test_end; j++, k++) {
        test_cmd[k] = strdup(command[j]);
    }
    test_cmd[test_token_count] = NULL;

    //si TEST est une commande structurée
    if (!is_valid_test_condition(test_cmd, test_token_count)) {
        write(STDERR_FILENO, "Fonctionnalité non gérée: 'TEST' ne peut pas contenir de commandes structurées\n", 83);
        // Libération de la mémoire
        for (int k = 0; k < test_token_count; k++) {
            free(test_cmd[k]);
        }
        free(test_cmd);
        return 2;
    }

    i++; // Passer '{'
    int cmd_start = i;

    // Gestion des accolades pour le bloc 'if'
    int brace_count = 1;
    while (command[i] != NULL && brace_count > 0) {
        if (strcmp(command[i], "{") == 0) {
            brace_count++;
        } else if (strcmp(command[i], "}") == 0) {
            brace_count--;
            if (brace_count == 0) {
                break;
            }
        }
        i++;
    }

    if (brace_count != 0) {
        write(STDERR_FILENO, "Erreur de syntaxe : accolades manquantes ou mal placées dans le bloc 'if'.\n", 76);
        // Libération de la mémoire
        for (int k = 0; test_cmd[k] != NULL; k++) {
            free(test_cmd[k]);
        }
        free(test_cmd);
        return 2;
    }

    int cmd_end = i - 1; // Dernier token avant '}'

    // Conversion des tokens du bloc 'if' en une chaîne de caractères
    char *cmd_if_str = tokens_to_string(&command[cmd_start], cmd_end - cmd_start + 1);

    i++; // Passer '}'

    // Vérifier la présence d'un 'else' ou de tokens supplémentaires
    bool has_else = false;
    char *cmd_else_str = NULL;

    if (command[i] != NULL && strcmp(command[i], "else") == 0) {
        i++; // Passer 'else'

        if (command[i] == NULL || strcmp(command[i], "{") != 0) {
            write(STDERR_FILENO, "Erreur de syntaxe : '{' manquant après 'else'.\n", 48);
            free(cmd_if_str);
            for (int k = 0; test_cmd[k] != NULL; k++) {
                free(test_cmd[k]);
            }
            free(test_cmd);
            return 2;
        }

        i++; // Passer '{'
        int else_start = i;

        // Gestion des accolades pour le bloc 'else'
        brace_count = 1;
        while (command[i] != NULL && brace_count > 0) {
            if (strcmp(command[i], "{") == 0) {
                brace_count++;
            } else if (strcmp(command[i], "}") == 0) {
                brace_count--;
                if (brace_count == 0) {
                    break;
                }
            }
            i++;
        }

        if (brace_count != 0) {
            write(STDERR_FILENO, "Erreur de syntaxe : accolades manquantes ou mal placées dans le bloc 'else'.\n", 75);
            free(cmd_if_str);
            for (int k = 0; test_cmd[k] != NULL; k++) {
                free(test_cmd[k]);
            }
            free(test_cmd);
            return 2;
        }

        int else_end = i - 1; // Dernier token avant '}'

        // Conversion des tokens du bloc 'else' en une chaîne de caractères
        cmd_else_str = tokens_to_string(&command[else_start], else_end - else_start + 1);
        has_else = true;

        i++; // Passer '}'
    }

    // Vérifier s'il reste des tokens après le bloc 'if' ou 'else'
    if (command[i] != NULL) {
        write(STDERR_FILENO, "Erreur de syntaxe : tokens supplémentaires après la fin de la commande\n", 73);
        free(cmd_if_str);
        if (has_else) {
            free(cmd_else_str);
        }
        for (int k = 0; test_cmd[k] != NULL; k++) {
            free(test_cmd[k]);
        }
        free(test_cmd);
        return 2;
    }

    // Exécution de la commande de test sans afficher la sortie
    int test_result = execute_command(test_cmd);

    //Si execute_command retourne une erreur de syntaxe
    if (test_result == 2) {
        // Libération de la mémoire
        for (int k = 0; test_cmd[k] != NULL; k++) {
            free(test_cmd[k]);
            }
        free(test_cmd);
        free(cmd_if_str);
        if (has_else) {
            free(cmd_else_str);
        }
        return 2;
    }

    // Libération de la mémoire des tokens de test
    for (int k = 0; test_cmd[k] != NULL; k++) {
        free(test_cmd[k]);
    }
    free(test_cmd);

    int return_val = 0;
    if (test_result == 0) {
        // Exécuter le bloc 'if'
        return_val = execute_block(cmd_if_str);
    } else if (has_else) {
        // Exécuter le bloc 'else'
        return_val = execute_block(cmd_else_str);
    } else {
        // Si pas de 'else' et condition fausse, retourner 0 et la condition est syntaxiquement correcte
        return_val = 0;
    }

    // Libération de la mémoire
    free(cmd_if_str);
    if (has_else) {
        free(cmd_else_str);
    }

    return return_val;
}