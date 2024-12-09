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
#include "../headers/handler.h" // pour execute_command
#include "../headers/prompt.h" // pour last_status

// Macros
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int for_syntax (char** command, int optindex) {
        // Vérification de la syntaxe de la commande for
    if (command[0] == NULL || strcmp(command[0], "for") != 0) {
        write(STDERR_FILENO, "Erreur : la commande doit commencer par 'for'\n", 47);
        return 1;
    }
    if (command[1] == NULL){
        // La variable de boucle
        write(STDERR_FILENO, "Erreur : variable manquante après 'for'\n", 41);
        return 1;
    }
    if (strlen(command[1]) != 1) {
        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer), "Erreur : la variable de boucle '%s' doit être limitée à un caractère\n", command[1]);
        write(STDERR_FILENO, buffer, len);
        return 1;
    }
    if (command[2] == NULL  || strcmp(command[2], "in") !=  0) {
        // Mot-clé 'in'
        write(STDERR_FILENO, "Erreur : mot-clé 'in' manquant après la variable de boucle ou espaces incorrects\n", 83);
        return 1;
    }
    if(command[3] == NULL) {
        // Le répertoire
        write(STDERR_FILENO, "Erreur : répertoire cible manquant après 'in'\n", 48);
        return 1;
    }
    if (command[optindex] == NULL || strcmp(command[optindex], "{") != 0) {
        // Accolade ouvrante {
        write(STDERR_FILENO, "Erreur : '{' manquante ou mal positionnée pour ouvrir la commande structurée \n", 80);
        return 1;
    } else {
        // La fin par '}'
        int i = optind + 1;
        while (command[i] != NULL) {
            if (strcmp(command[i], "}") == 0) {
                break;
            }
            i++;
        }
        if (command[i] == NULL) {
            write(STDERR_FILENO, "Erreur : '}' manquant pour fermer la commande structurée ou espaces incorrects\n", 80);
            return 1;
        } 
        return 0;
    }
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

    if (dp == NULL) {
        perror("opendir");
        return 2;
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


        // type check
        if ((!type && S_ISREG(file_stat.st_mode) )|| (type && TYPE == 'f' && S_ISREG(file_stat.st_mode))) return_val = MAX(return_val, do_for(command, optindex, var, full_path, extension));
        if (type && TYPE == 'd' && S_ISDIR(file_stat.st_mode)) return_val = MAX(return_val, do_for(command, optindex, var, full_path, extension));
        if (type && TYPE == 'l' && S_ISLNK(file_stat.st_mode)) return_val = MAX(return_val, do_for(command, optindex, var, full_path, extension));
        if (type && TYPE == 'p' && S_ISFIFO(file_stat.st_mode)) return_val = MAX(return_val, do_for(command, optindex, var, full_path, extension));

        // recursion check
        if (recursive && S_ISDIR(file_stat.st_mode)) {
            // Skip "." and ".." directories
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                return_val = MAX(return_val, browse_directory(full_path, hidden, recursive, extension, EXT, type, TYPE, var, command, optindex, return_val));
            }
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

int for_loop(char** command){
    int argc = 0;
    int opt = 0;
    int recursive = 0, hidden =0, extension=0, type=0, parallelism = 0;
    char* EXT, TYPE;

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
                // MAX_THREADS = optarg;
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
    char* test[10] = {0};
    int i = 1;
    int j = 0;
    while (command[i] != NULL && strcmp(command[i], "{") != 0) {
        test[j] = command[i];
        i++;
        j++;
    }
    // check for missing {
    if (command[i] == NULL) {
        write(STDERR_FILENO, "Syntax error : missing '{'.\n", 29);
        return 1;
    }
    test[j] = NULL; 

    // debug print test
    // printf("test\n");
    // for (int k = 0; k < 10; k++) {
    //     if (test[k] != NULL) {
    //         printf("%s ", test[k]);
    //     }
    //     printf("\n");
    // }

    // find cmd1 delimited by { }
    char* cmd1[10] = {0};
    i++; // Skip the '{'
    j = 0;
    while (command[i] != NULL && strcmp(command[i], "}") != 0) {
        cmd1[j] = command[i];
        i++;
        j++;
    }
    // check for missing }
    if (command[i] == NULL) {
        write(STDERR_FILENO, "Syntax error : missing '}.'\n", 29);
        return 1;
    }
    cmd1[j] = NULL; 

    // debug 
    // printf("cmd1\n");
    // for (int k = 0; k < 10; k++) {
    //     if (cmd1[k] != NULL) {
    //         printf("%s ", cmd1[k]);
    //     }
    //     printf("\n");
    // }

    // check for else
    bool has_else = false;
    if (command[i+1] != NULL && strcmp(command[i+1], "else") == 0) {
        has_else = true;
    }

    // find cmd2 delimited by { }
    char* cmd2[10] = {0};
    if (has_else) {
        // check for missing {
        if (command[i+2] == NULL || strcmp(command[i+2], "{") != 0) {
            write(STDERR_FILENO, "Syntax error : missing '{' after else.\n", 39);
            return 1;
        }

        i += 3; // Skip the '} else {'
        j = 0;
        while (command[i] != NULL && strcmp(command[i], "}") != 0) {
            cmd2[j] = command[i];
            i++;
            j++;
        }
        cmd2[j] = NULL; 

        // check for missing }
        if (command[i] == NULL) {
            write(STDERR_FILENO, "Syntax error : missing '}.'\n", 29);
            return 1;
        }
    }

    // debug 
    // printf("cmd2\n");
    // for (int k = 0; k < 10; k++) {
    //     if (cmd2[k] != NULL) {
    //         printf("%s ", cmd2[k]);
    //     }
    //     printf("\n");
    // }

    // Execute the test command
    
    // debug
    // printf("last_status: %d\n", last_status);
    if (execute_command(test) == 0) {
        return execute_command(cmd1);
    } else if (has_else) {
        return execute_command(cmd2);
    }


    return 0;
}