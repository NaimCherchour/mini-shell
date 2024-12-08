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

int for_syntax (char** command, int optind) {
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
    if (command[optind] == NULL || strcmp(command[optind], "{") != 0) {
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

void browse_directory(const char *directory, int hidden, int recursive, char var, char *cmd, char **command, int optind) {
    struct dirent *entry;
    DIR *dp = opendir(directory);

    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        // Skip hidden files and directories (those starting with '.')
        if (!hidden && entry->d_name[0] == '.') {
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
            // Construct new char array to store the command and the file
            char* new_command[10] = {0};
            new_command[0] = cmd;
            size_t i = 1;
            while (command[i+optind] != NULL && i < 10 && strcmp(command[i+optind], "}") != 0) {
                if (command[i+optind][0] == '$' && command[i+optind][1] == var) {
                    new_command[i] = full_path;
                    i++;
                    continue;
                }
                new_command[i] = command[i+optind];
                i++;
            }

            // Handle the command
            execute_command(new_command);
        }

        // If recursive flag is set and entry is a directory, browse subdirectory
        if (recursive && S_ISDIR(file_stat.st_mode)) {
            // Skip "." and ".." directories
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                browse_directory(full_path, hidden, recursive, var, cmd, command, optind);
            }
        }
    }

    if (closedir(dp) == -1) {
        perror("closedir");
        return;
    }
}

int for_loop(char** command){

    int argc = 0;
    int opt = 0;
    int recursive, hidden, extension, type, parallelism = 0;

    // calculate argc
    while (command[argc] != NULL) {
        argc++;
    }
    
    char var = command[1][0]; // variable (only one letter)
    char* directory = command[3]; 

    optind = 4;

    while ((opt = getopt(argc, command, "Aretp")) != -1) {
        switch (opt) {
            case 'A':
                hidden = 1;
                // printf("option A: hidden = %d\n", hidden);
                break;
            case 'r':
                recursive = 1;
                // printf("option r: recursive = %d\n", recursive);
                break;
            case 'e':
                extension = 1;
                printf("option e: extension = %d\n", extension);
                break;
            case 't':
                type = 1;
                printf("option t: type = %d\n", type);
                break;
            case 'p':
                parallelism = 1;
                printf("option p: parallelism = %d\n", parallelism);
                break;
            case '?':
                write(STDERR_FILENO, "Usage: for f in dir [-A] [-r] [-e] [-t] [-p] { cmd $f }\n", 56);
                return 1;
        }
    }

    // syntax check
    if (for_syntax(command, optind) == 1 ) return 1 ;
    // command[optind] is "{"
    char* cmd = command[optind+1]; // command to execute

    // Browse the directory
    // printf("directory: %s\n", directory);
    // printf("hidden: %d\n", hidden);
    // printf("recursive: %d\n", recursive);
    // printf("var: %c\n", var);
    // printf("cmd: %s\n", cmd);

    browse_directory(directory, hidden, recursive, var, cmd, command, optind+1);

    return EXIT_SUCCESS; 
    
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