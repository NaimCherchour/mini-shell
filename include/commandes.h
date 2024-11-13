#ifndef COMMANDE_H
#define COMMANDE_H

#include <limits.h>  // pour PATH_MAX

// Déclaration de la variable globale pour le répertoire précédent
extern char rep_precedent[PATH_MAX]; 

// Déclaration de la fonction cd qui change de répertoire qui devient le répertoire REF (référence valide), le précédent répertoire de travail
// si le paramètre est -, ou $HOME en l'absence de paramètre.
int cd(char **args);

//Affiche le type du fichier de référence REF (s'il s'agit d'une référence valide) : 
//directory, regular file , symbolic link, named pipe, other.
int ftype(char **args);

int pwd();  // Affiche le répertoire courant



#endif // COMMANDE_H