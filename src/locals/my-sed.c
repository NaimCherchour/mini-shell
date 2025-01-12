#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>

#include <stdio.h>

#define BUFFER_SIZE 10

char usage[] = " prend trois paramètres\n";

int do_sed(int fd, int del_char, char car1, char car2);

/* ./my-sed car1 car2 filename remplace chaque occurrence du caractère
 * car1 par car2 dans le fichier de référence filename.  
 * ./my-sed -d car1 filename supprime les occurrences du caractère car1
 * dans le fichier de référence filename. */
int main(int argc, char * argv[]) {
  int result;
  int del_char = 0;
  char car1, car2 = '\0';
  int fd;

  if(argc != 4){ 
    write(STDERR_FILENO, argv[0], strlen(argv[0]));
    write(STDERR_FILENO, usage, strlen(usage));
    result = 1;
    goto cleanup_return;
  }
	
  int opt;
  if ((opt = getopt(argc, argv, "d")) != -1) {
    /* TODO: gérer l'option -d */
   if (strncmp(argv[1], "-d", 2) == 0) {
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

  /* TODO: ouvrir en lecture/écriture le fichier dont la référence est 
   * passée en paramètre. L'ouverture échoue si le fichier n'est pas
   * accessible */

  fd = open(argv[3], O_RDWR);
  if (fd == -1) {
    result =1;
    goto cleanup_return;
  }
	
  /* TODO: appeler do_sed avec les bons arguments */

  result = do_sed(fd, del_char, car1, car2);

  /* TODO: fermer le fichier */
  if ((close(fd) != 0)) {
    result =  1;
    goto cleanup_return;
  }
	
cleanup_return:
  return result;
}


int do_sed(int fd, int del_char, char car1, char car2) {
  int result = 0;
  char * buf_in = NULL, * buf_out = NULL; /* buffers de lecture et d'écriture */
  int lect = 0, ecr = 0; /* respectivement nombre d'octets lus et nombre
                            d'octets écrits dans le fichier */

  /* TODO: allouer les buffers de lecture et d'écriture */
  buf_in = (char *)malloc(BUFFER_SIZE);
  if (!buf_in) {
    result = 1;
    goto cleanup_return;
  }
  buf_out = (char *)malloc(BUFFER_SIZE);
  if (!buf_out) {
    result = 1;
    goto cleanup_return;
  }


  while(1) {
    /* TODO: placer la tête de lecture au bon endroit */
    lseek(fd, lect, SEEK_SET);

    /* TODO:  lire dans le fichier (tant qu'il y a quelque chose à lire) */
    int nbr = read(fd, buf_in, BUFFER_SIZE);
    if (nbr < 0) {
      result = 1;
      goto cleanup_return;
    }
    if (nbr == 0) break;

    /* TODO: mettre à jour la variable lect */
    lect += nbr;

    /* TODO: écrire dans le buffer d'écriture l'entrée modifiée selon la
     * valeur de del_char */
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
		
    /* TODO: placer la tête d'écriture au bon endroit */
    lseek(fd, ecr, SEEK_SET);

    /* TODO: écrire le résultat dans le fichier */
    int nbw = write(fd, buf_out, strlen(buf_out));
    if (nbw < 0) {
      result = 1;
      goto cleanup_return;
    }
		
    /* TODO: mettre à jour la variable ecr */
    ecr += nbw;
		
  }

  if (del_char && ecr < lect) {
    /* TODO: tronquer la fin du fichier */
    if (ftruncate(fd, ecr) == -1) {
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

