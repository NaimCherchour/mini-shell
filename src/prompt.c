#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

// headers
#include "prompt.h" 

int last_status = 0; // Variable contenant la dernière valeur de retour ou 255

char *generate_prompt() {
    char status_color[16];
    char status_text[16];
    char *cwd;
    char *cwd_display;
    char *prompt;


    if (last_status < 0) {
        snprintf(status_text, sizeof(status_text), "SIG");
        strncpy(status_color, COLOR_RED, sizeof(status_color));
    } else {
        snprintf(status_text, sizeof(status_text), "%d", last_status);
        strncpy(status_color, last_status == 0 ? COLOR_GREEN : COLOR_RED, sizeof(status_color)); // vert cas 0 et rouge pour autre valeur 
    }

    // Répertoire courant
    char path[PATH_MAX];
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    // Raccourcir le chemin avec ~ si HOME est préfixe ( AJOUT )
    char *home = getenv("HOME");
    if (home && strncmp(path, home, strlen(home)) == 0) {
        size_t len_home = strlen(home);
        cwd = malloc(strlen(path) - len_home + 2); // ~/ + reste du chemin
        sprintf(cwd, "~%s", path + len_home);
    } else {
        cwd = strdup(path);
    }

    // Tronquer le chemin si nécessaire
    size_t max_cwd_length = 30 - strlen(status_text) -4; // [] + last_status + "$ " 
    if (strlen(cwd) > max_cwd_length) {
        size_t len_to_copy = max_cwd_length - 3; // ...chemin
        cwd_display = malloc(max_cwd_length + 1);
        snprintf(cwd_display, max_cwd_length + 1, "...%s", cwd + strlen(cwd) - len_to_copy);
        free(cwd);
    } else {
        cwd_display = cwd; 
    }

    // Ajout des couleurs
    size_t cwd_with_color_len = strlen(cwd_display) + strlen(COLOR_BLUE) + strlen(COLOR_RESET) + 1;
    char *cwd_with_color = malloc(cwd_with_color_len);
    snprintf(cwd_with_color, cwd_with_color_len, "%s%s%s", COLOR_BLUE, cwd_display, COLOR_RESET);
    free(cwd_display);

    // Prompt complet
    size_t prompt_len = strlen(status_color) + strlen(status_text) + strlen(cwd_with_color) + strlen(COLOR_RESET) + 4;
    prompt = malloc(prompt_len);
    snprintf(prompt, prompt_len, "%s[%s]%s$ ", status_color, status_text, cwd_with_color);

    free(cwd_with_color);
    return prompt;
}