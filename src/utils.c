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


#include "../headers/utils.h"
#include "../headers/handler.h" // pour handle_commands
#include "../headers/prompt.h" // pour last_status

// Macros
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

int do_for(char** command, int optindex, char var, char* full_path, int extension) {
    char** new_command = constructor(command, optindex, full_path, var, extension);
    int return_val = execute_command(new_command);

    for (size_t i = 1; new_command[i] != NULL; i++) {
            free(new_command[i]);
    }
    free(new_command);

    return return_val;
}

int browse_directory(const char *directory, int hidden, int recursive, int extension, char* EXT, int type, char TYPE, char var, char **command, int optindex, int return_val) {
    struct dirent *entry;
    DIR *dp = opendir(directory);
    int result = return_val;

    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    while ((entry = readdir(dp)) != NULL) {
        // hidden files check
        if (!hidden && entry->d_name[0] == '.') {
            continue;
        }

        // extension check
        if (extension) {
            const char* dot = strrchr(entry->d_name, '.');
            if (!dot || strcmp(EXT, dot+1) != 0) continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        // Get file information
        struct stat file_stat;
        if (lstat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        // skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }


        // type check
        if ((!type )|| 
        (type && TYPE == 'f' && S_ISREG(file_stat.st_mode)) || 
        (type && TYPE == 'd' && S_ISDIR(file_stat.st_mode)) ||
        (type && TYPE == 'l' && S_ISLNK(file_stat.st_mode)) || 
        (type && TYPE == 'p' && S_ISFIFO(file_stat.st_mode)) ) {
            result =  do_for(command, optindex, var, full_path, extension);
            return_val = MAX(return_val, result);
        }

        // recursion check
        if (recursive && S_ISDIR(file_stat.st_mode)) {
            result = browse_directory(full_path, hidden, recursive, extension, EXT, type, TYPE, var, command, optindex, return_val);
            return_val = MAX(return_val, result);
        }
    }

    if (closedir(dp) == -1) {
        perror("closedir");
        return 2;
    }

    return return_val;
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

char** constructor(char** command, int optindex, char* full_path, char var, int extension) {
    // Allocate memory for the new command array
    char** new_command = malloc(10 * sizeof(char*));
    if (new_command == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    new_command[0] = command[optindex + 1];

    size_t i = 1;
    char var_str[3] = {'$', var, '\0'}; 

    if (extension) {
    char* dot = strrchr(full_path, '.');
    if (dot) {
        *dot = '\0';  // Truncate the string at the dot
    }
}

    while (command[i + optindex + 1] != NULL && i < 10 && strcmp(command[i + optindex + 1], "}") != 0) {  
        new_command[i] = replace_var_with_path(command[i + optindex + 1], var_str, full_path);
        i++;
    }

    new_command[i] = NULL; // Null-terminate the array
    return new_command;
}


// Méthode interne 
int execute_block(char* block_command) {
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


int for_loop(char** command){
    int argc = 0;
    int opt = 0;
    int recursive = 0, hidden =0, extension=0, type=0, parallelism = 0;
    char* EXT, TYPE, *MAX_THREADS;

    // calculate argc
    while (command[argc] != NULL) {
        argc++;
    }

    if (argc < 4) {
        write(STDERR_FILENO, "Usage: for f in dir [-A] [-r] [-e] [-t] [-p] { cmd $f }\n", 56);
        return 1;
    }
    
    char var = command[1][0]; // variable (only one letter)
    char* directory = command[3]; 

    optind = 4;

    while ((opt = getopt(argc, command, "Are:t:p:")) != -1) {
        switch (opt) {
            case 'A':
                hidden++;
                break;
            case 'r':
                recursive++;
                break;
            case 'e':
                extension++;
                EXT = optarg;
                break;
            case 't':
                type++;
                TYPE = optarg[0];
                if (TYPE != 'd' && TYPE != 'f' && TYPE != 'l' && TYPE != 'p') {
                    write(STDERR_FILENO, "Error: type must be 'd' or 'f' or 'l' or 'p'\n", 45);
                    return 1;
                }
                break;
            case 'p':
                parallelism++;
                MAX_THREADS = optarg;
                break;
            case '?':
                write(STDERR_FILENO, "Usage: for f in dir [-A] [-r] [-e] [-t] [-p] { cmd $f }\n", 56);
                return 1;
        }
        
    }


    // syntax check
    if (for_syntax(command, optind) == 1 ) return 1 ;

    return browse_directory(directory, hidden, recursive, extension, EXT, type, TYPE, var, command, optind, 0);

    
}


// if TEST { CMD } else { CMD }
// TEST is a pipeline of commands that return 0 or 1
int if_else(char** command) {
    // find test delimited by if and {
    int i = 1;
    int test_start = i; 
    while (command[i] != NULL && strcmp(command[i], "{") != 0) {
        i++;
    }
    // check for missing {
    if (command[i] == NULL) {
        write(STDERR_FILENO, "Syntax error : missing '{'.\n", 29);
        return 2; //TODO : à vérifier si c'est la bonne valeur de retour pour les erreurs de syntaxe ( serveur discord )
    }
    int test_end = i - 1; 

    // find cmd delimited by { }
    char** test_cmd = malloc((test_end - test_start + 2) * sizeof(char *));
    for (int j = test_start, k = 0; j <= test_end; j++, k++) {
        test_cmd[k] = strdup(command[j]); 
    }
    test_cmd[test_end - test_start + 1] = NULL;
    
    i++; // Skip the '{'
    int cmd_start = i;
    int bracket_count = 1;
    while (command[i] != NULL && bracket_count > 0) {
         if (strcmp(command[i], "{") == 0) {
            bracket_count++;
        } else if (strcmp(command[i], "}") == 0) {
            bracket_count--;
        }
        i++;
    }
    if (bracket_count != 0) {
        write(STDERR_FILENO, "Erreur syntaxique\n", 19); // }
        return 2;
    }
    int cmd_end = i - 2;

    //On converti les tokens en chaîne 
    char *cmd_if_str = tokens_to_string(&command[cmd_start], cmd_end - cmd_start + 1);

    // check for else
    bool has_else = false;
    char *cmd_else_str = NULL; 
    if (command[i] != NULL && strcmp(command[i], "else") == 0) {
        i++; // Passe le 'else'
        if (command[i] == NULL || strcmp(command[i], "{") != 0) {
            write(STDERR_FILENO, "Syntax error : missing '{' after else.\n", 39);
            return 2;
        }
        i++; // Passer '{'
        int cmd_else_start = i;
        bracket_count = 1;
        while (command[i] != NULL && bracket_count > 0) {
            if (strcmp(command[i], "{") == 0) {
                bracket_count++;
            } else if (strcmp(command[i], "}") == 0) {
                bracket_count--;
            }
            i++;
        }
        if (bracket_count != 0) {
            write(STDERR_FILENO, "Erreur syntaxique\n", 19); // }
            return 2;
        }
        int cmd_else_end = i - 2;

        cmd_else_str = tokens_to_string(&command[cmd_else_start], cmd_else_end - cmd_else_start + 1);
        has_else = true;
    }

    // On analyse et exécute la commande test
    int test_result = execute_command(test_cmd);
    //On libère la mémoire du test
    for (int j = 0; test_cmd[j] != NULL; j++) {
        free(test_cmd[j]);
    }
    free(test_cmd);

    int return_val = 0;
    if (test_result == 0) {
        return_val = execute_block(cmd_if_str); // on exécute le block du if ( commande simple ou structurée )
    } else if (has_else) {
        return_val = execute_block(cmd_else_str); // même chose pour le block else
    }

    //On LIBÈRE la mémoire
    free(cmd_if_str); 
    if (cmd_else_str) {
        free(cmd_else_str);
    }


    return return_val;
}