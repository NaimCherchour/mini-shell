#ifndef INTERNALS_H
#define INTERNALS_H

// Déclaration de la variable globale pour le répertoire précédent
extern char rep_precedent[PATH_MAX]; 

// Calcul du le nombre d'arguments
int nb_arguments (char **args );

// Déclaration de la fonction cd qui change de répertoire qui devient le répertoire REF (référence valide), le précédent répertoire de travail
// si le paramètre est -, ou $HOME en l'absence de paramètre.
int cd(char **args);

//Affiche le type du fichier de référence REF (s'il s'agit d'une référence valide) : 
//directory, regular file , symbolic link, named pipe, other.
int ftype(char **args);

int pwd(char **args);  // Affiche le répertoire courant

int exit_shell(char **args);  // Termine le shell avec un code de sortie



#endif