#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>

#include <stdio.h>

#define BUFFER_SIZE 512  // Réduction de la taille pour optimiser la mémoire


char usage[] = " prend deux paramètres\n";

int do_tr(int del_char, char car1, char car2);

/* ./my-tr car1 car2 recopie l'entrée standard sur la sortie standard en
 * remplaçant chaque occurrence du caractère car1 par car2.  
 * ./my-tr -d car1 recopie l'entrée standard sur la sortie standard en 
 * supprimant les occurrences du caractère car1. */
int main(int argc, char * argv[]) {
  int result;
  int del_char = 0;
  char car1, car2 = '\0'; 

  if(argc != 3){ 
    write(STDERR_FILENO, argv[0], strlen(argv[0]));
    write(STDERR_FILENO, usage, strlen(usage));
    result = 1;
    goto cleanup_return;
  }

  int opt;
  if ((opt = getopt(argc, argv, "d")) != -1) {
    /* TODO: gérer l'option -d */
    if (opt == 'd') {
      del_char = 1;
      car1 = argv[2][0];
    }
    else {
      result = 1;
      goto cleanup_return;
    }
  }
  else {
    /* TODO: gérer le cas sans option */
    car1 = argv[1][0];
    car2 = argv[2][0];
  }

  /* TODO: appeler do_tr avec les bons arguments */
  result = do_tr(del_char, car1, car2);

cleanup_return:
  return result;
}

/* recopie l'entrée standard sur la sortie standard en :
   - remplaçant chaque occurrence du caractère car1 par car2 si del_char = 0,
   - supprimant les occurrences du caractère car1 si del_char = 1 */
int do_tr(int del_char, char car1, char car2) {
  int result = 0;
  char * buf_in = NULL, * buf_out = NULL; /* buffers de lecture et d'écriture */

  /* TODO: allouer les buffers de lecture et d'écriture */
  buf_in = (char *)malloc(BUFFER_SIZE);
if (!buf_in) {
    perror("malloc");
    return 1;
}
buf_out = (char *)malloc(BUFFER_SIZE);
if (!buf_out) {
    perror("malloc");
    free(buf_in);
    return 1;
}

  
  while(1) {
    /* TODO: lire l'entrée (tant qu'il y a quelque chose à lire) */
    int nbr = read(STDIN_FILENO, buf_in, BUFFER_SIZE);
    if (nbr < 0) {
      result = 1;
      goto cleanup_return;
    }
    if (nbr == 0) break;

    /* TODO: écrire dans le buffer d'écriture l'entrée modifiée
     * selon la valeur de del_char */
    if (!del_char) {
      for (size_t i = 0; i < nbr; i++){
        buf_out[i] = buf_in[i];
        if (buf_in[i] == car1) buf_out[i] = car2;
      }
      buf_out[nbr] = '\0';
    } else{
      size_t j = 0;
       for (size_t i = 0; i < nbr; i++){
        if (buf_in[i] == car1) {
          continue;
        }
        buf_out[j] = buf_in[i];
        ++j;
      } 
      buf_out[j] = '\0';
    }
		
    /* TODO: écrire le résultat sur la sortie standard */
    int nbw = write(STDOUT_FILENO, buf_out, strlen(buf_out));
    if (nbw < 0) {
      result = 1;
      goto cleanup_return;
    }
	 
  }
  
cleanup_return:
  /* TODO: libérer les buffers */
  if (buf_in) free(buf_in);
  if (buf_out) free(buf_out);
	
  return result;
}

