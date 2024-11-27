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


#include "../headers/utils.h"
#include "../headers/handler.h" // pour handle_command

int for_loop_valide_syntax ( char** command ) {
        // Vérification de la syntaxe de la commande for
    if (command[0] == NULL || strcmp(command[0], "for") != 0) {
        fprintf(stderr, "Erreur : la commande doit commencer par 'for'\n");
        return 1;
    } else if (command[1] == NULL){
        // La variable de boucle
        fprintf(stderr, "Erreur : variable manquante après 'for'\n");
        return 1;
    } else if (strlen(command[1]) != 1) {
        fprintf(stderr, "Erreur : la variable de boucle '%s' doit être limité à un caractère\n", command[1]);
        return 1;
    } else if (command[2] == NULL  || strcmp(command[2], "in") !=  0) {
          // Mot-clé 'in'
        fprintf(stderr, "Erreur : mot-clé 'in' manquant après la variable de boucle ou espaces incorrects\n");
        return 1;
    } else if(command[3] == NULL) {
        // Le répertoire
        fprintf(stderr, "Erreur : répertoire cible manquant après 'in'\n");
        return 1;
    } else if (command[4] == NULL || strcmp(command[4], "{") != 0) {
        // Accolade ouvrante {
        fprintf(stderr, "Erreur : '{' manquante ou mal positionnée pour ouvrir la commande structurée \n");
        return 1;
    } else {
        // La fin par '}'
        int i = 5;
        while (command[i] != NULL) {
            if (strcmp(command[i], "}") == 0) {
                break;
            }
            i++;
        }
        if (command[i] == NULL) {
            fprintf(stderr, "Erreur : '}' manquant pour fermer la commande structurée ou espaces incorrects\n");
            return 1;
        } else { return 0 ;}
    }
}

int for_loop(char** command){

    if (for_loop_valide_syntax(command) == 1 ) {
            return 1 ;
    } else {
        // Syntaxe valide de for
        char var = command[1][0]; // variable (only one letter)
        char* directory = command[3]; 
        char* cmd = command[5];

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
                char* new_command[10] = {0};
                new_command[0] = cmd;
                size_t i = 1;
                while (command[i+5] != NULL && i < 10 && strcmp(command[i+5], "}") != 0) {
                    if (command[i+5][0] == '$' && command[i+5][1] == var) {
                        new_command[i] = full_path;
                        i++;
                        continue;
                    }
                    new_command[i] = command[i+5];
                    i++;
                }

                // Handle the command
                handle_command(new_command);
            }
    
        }
        
        closedir(dp);
        return 0;
    }
}