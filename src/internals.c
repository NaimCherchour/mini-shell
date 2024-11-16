#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "../headers/internals.h"

char rep_precedent[PATH_MAX]; // on sauvegarde le répertoire précédent

int cd (char **args) {

    char *rep_destinataire;
    char rep_courant[PATH_MAX];

    if (args[1] == NULL) {     // Si l'utilisateur entre que "cd", on va vers le répertoire HOME
        rep_destinataire = getenv("HOME");
        if (rep_destinataire == NULL) {
            fprintf(stderr, "cd: HOME non définie \n");
            return 1;
        }
    } else if (strcmp(args[1], "-") == 0) {     // Si l'argument est "-", on va vers le répertoire précédent
        if (rep_precedent[0] == '\0') {
            fprintf(stderr, "cd: répertoire précédent non défini \n");
            return 1;
        }
        rep_destinataire = rep_precedent;
    }
    // Sinon, on va vers le répertoire spécifié par l'utilisateur
    else {
        rep_destinataire = args[1];
    }

    // On sauvegarde le répertoire courant avant de changer de répertoire
    if (getcwd(rep_courant, sizeof(rep_courant)) == NULL) {
        perror("getcwd");
        return 1;
    }
    // Changement de répertoire
    if (chdir(rep_destinataire) != 0) {
        perror("cd");
        return 1;
    }

    // Mise à jour du répertoire précédent
    strncpy(rep_precedent, rep_courant, sizeof(rep_precedent));

    return 0;
}

int ftype(char **args) {
    struct stat file_stat;

    if (args[1] == NULL) {
        fprintf(stderr, "ftype: paramètre manquant\n");
        return 1;
    }
    if (lstat(args[1], &file_stat) == -1) {
        perror("ftype");
        return 1;
    }
    if (S_ISDIR(file_stat.st_mode)) {
        printf("directory\n");
    } else if (S_ISREG(file_stat.st_mode)) {
        printf("regular file\n");
    } else if (S_ISLNK(file_stat.st_mode)) {
        printf("symbolic link\n");
    } else if (S_ISFIFO(file_stat.st_mode)) {
        printf("named pipe\n");
    } else {
        printf("other\n");
    }

    return 0;
}

int pwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);  // Affiche le répertoire courant
        return 0;
    } else {
        perror("pwd");  // Affiche une erreur si `getcwd` échoue
        return 1;
    }
}

void exit_shell(char **args) {
    int exit_code = 0; // Code de sortie par défaut

    if (args[1] != NULL) {
        exit_code = atoi(args[1]); // Convertit le second argument en entier
    }

    printf("Exiting shell with code %d\n", exit_code);
    exit(exit_code);
}
