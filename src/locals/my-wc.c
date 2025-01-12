#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>

#define BUFFER_SIZE 1024

int do_wc(int in_fd, int out_fd, int write_newlines, int write_bytes, const char * in_filename);
int print_size(int fd, size_t size);

/* ./my-wc [-lc] [fichier] compte le nombre de retours à la ligne et/ou
 * d'octets dans le fichier indiqué, ou à défaut sur l'entrée standard. */
int main(int argc, char * argv[]) {
  int result = 0;
  char * in_filename = NULL;
  int write_newlines = 0;
  int write_bytes = 0;
  
  int in_fd = STDIN_FILENO;

  int opt;
  while ((opt = getopt(argc, argv, "cl")) != -1) {
    /* on gère les options -c et -l */
    switch (opt) {
      case 'l':
        write_newlines = 1;
        break;
      case 'c':
        write_bytes = 1;
        break;
      case '?':
        result = 1;
        goto cleanup_return;
        break;
    }
  }
  /* si ni -c ni -l n'a été donnée, on fait en sorte d'afficher les deux */
  if (!write_newlines && !write_bytes) {
      write_newlines = 1;
      write_bytes = 1;
  }

  /* on teste si un nom de fichier a été donné (cf. man getopt -> optind) 
   * et dans ce cas, on le met dans in_filename */
  if (optind < argc) in_filename = argv[optind];

  /* si un nom de fichier autre que "-" a été donné, ouvrir le fichier 
   * correspondant */
  if (in_filename && strcmp(in_filename, "-") != 0) {
    in_fd = open(in_filename, O_RDONLY);
    if (in_fd == -1) {
      perror("open");
      result = 1;
      goto cleanup_return;
    }
  }

  /* appeler do_wc avec les bons arguments */
  result = do_wc(in_fd, STDOUT_FILENO, write_newlines, write_bytes, in_filename);

cleanup_return:
  /* fermer le fichier d'entrée. Ne pas oublier de tester s'il y a eu 
   * une erreur */
  if (in_fd != STDIN_FILENO && close(in_fd) == -1) {
    perror("close");
    result = 1;
  }

  return result;
}

/* Lit in_fd et compte le nombre de lignes et/ou d'octets lus. Affiche le
 * résultat sur out_fd, suivi de in_filename si différent de NULL, le
 * tout sous formes de colonnes séparées par une tabulation. Renvoie 0 en
 * cas de succès et 1 en cas d'erreur. */
int do_wc(int in_fd, int out_fd, int write_newlines, int write_bytes, const char * in_filename) {
  int result = 0;
  char * buffer = NULL;
  size_t newlines_count = 0;
  size_t bytes_count = 0;

  /* allouer un buffer */
  buffer = (char *)malloc(BUFFER_SIZE);
  if (!buffer) {
    perror("malloc");
    result = 1;
    goto cleanup_return;
  }
   
  while(1) {
    /* lire l'entrée et compter le nombre de retours à la ligne et 
     * d'octets */
    ssize_t bytes_read = read(in_fd, buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
      perror("read");
      result = 1;
      goto cleanup_return;
    }
    if (bytes_read == 0) break; // End of file

    bytes_count += bytes_read;
    for (ssize_t i = 0; i < bytes_read; ++i) {
      if (buffer[i] == '\n') {
        newlines_count++;
      }
    }
  }
  
  /* afficher ce qu'il faut */
  if (write_newlines) {
    dprintf(out_fd, "%zu", newlines_count);
  }
  if (write_bytes) {
    if (write_newlines) {
      dprintf(out_fd, "\t");
    }
    dprintf(out_fd, "%zu", bytes_count);
  }
  if (!write_newlines && !write_bytes) {
    dprintf(out_fd, "%zu\t%zu", newlines_count, bytes_count);
  }
  if (in_filename) {
    dprintf(out_fd, "\t%s", in_filename);
  }
  dprintf(out_fd, "\n");
  
  /* tout a fonctionné : mettre à jour result en conséquence */
  result = 0;
  
cleanup_return:
  /* libérer le buffer */
  if (buffer) {
    free(buffer);
  }

  return result;
}

/* Renvoie le nombre d'octets écrits, ou -1 en cas d'erreur. */
int print_size(int fd, size_t size) {
  char buffer[32];
  int len = snprintf(buffer, sizeof(buffer), "%zu", size);
  if (len < 0) {
    return -1;
  }
  return write(fd, buffer, len);
}