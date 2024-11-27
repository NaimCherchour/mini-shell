# Projet SYS5 mini Shell fsh


## Structure du dépot :

📦mini-shell  

┣ 📂bin/ ------------------------------------> Contient les binaires executables pour les commandes locales.  
┣ 📂obj/ -----------------------------------> Contient les fichiers .o┃
┣ 📂headers/ --------------------------------> Contient les fichiers d'en-tête  
┃   ┃ ┣ prompt.h
┃   ┃ ┣ internals.h
┃   ┃ ┣ handler.h
┃   ┃ ┗ utils.h 
┣ 📂src/ ------------------------------------> Contient les sources   
┃ ┣ 📂locals/  ----------------------------> Contient les sources des commandes locales  
┃ ┃ ┣ my-sed.c  
┃ ┃ ┣ my-tr.c  
┃ ┃ ┗ my-wlc.c  
┃ ┣ prompt.c ------------------------------> source de la fonction d'affichage du prompt  
┃ ┣ handler.c -----------------------------> source de la lecture de ligne de commandes et d'exécution 
┃ ┃                                          des commandes internes/externes
┃ ┣ internals.c ---------------------------> source des commandes internes
┃ ┣ utils.c -------------------------------> source des commandes structurées ( for ... )
┃ ┗ main.c --------------------------------> source du fsh  ( méthode principale )
┣ ARCHITECTURE.md  
┣ AUTHORS.md   
┣ Makefile  
┣ README.md  
┗ fsh ------------------------------------> executable du mini shell. Géneré après compilation



## Architecture Logicielle :
* ### Description générale :
   Le mini shell fsh est un shell minimaliste qui permet d'exécuter des commandes internes et externes. Il est capable de gérer les commandes internes suivantes : `sed`, `tr`, `wlc`,`cd`,`pwd`,`ftype` et `exit`. Il permet également de gérer les commandes externes en utilisant la fonction `execvp` et d'autres types de commandes structurées comme le `for`,`if else`. Le mini shell fsh est capable de gérer les redirections d'entrée et de sortie, les pipes et les signaux.

* ### Principe de fonctionnement :
   1. Le mini shell fsh lit une ligne de commande entrée par l'utilisateur.
   2. Il parse ensuite la ligne de commande avec la fonction `parse_command` pour extraire les arguments et les options.
   3. Traiter la commande avec la fonction `handle_command`. <u>ie</u>:  Il vérifie s'il s'agit d'une commande interne ou externe en procedant ainsi:
      1. <b>Priorité aux internes:</b> 
      2. <b>Recherche de commandes locales: </b>Recherche si un executable dans le répertoire `bin` portant le meme nom que la commande existe. Si oui, il s'agit d'une commande interne. Il crée un processus fils pour exécuter la commande locale.
      3. <b>Enfin:</b> si on trouve rien, c'est une commande externe. il crée un processus fils pour exécuter la commande externe.

* ### Fichiers source :
  1. <b>main</b> :
      contient la méthode principale main() qui initialise le shell et appelle la génération du prompt, lecture de la commande et puis on appelle la méthode pour l'exécution de la commande .
  2. <b>prompt</b> :
   Définit la génération du prompt dynamique en incluant la gestion de couleurs, affichage de last_status et du répertoire courant avec raccourcissement.
  3. <b>handler</b> :
   Implémente le parsing des commandes (parse_command), l’exécution des commandes internes comme cd, ftype, pwd, exit, et le contrôle des processus pour les commandes externes avec gestion des signaux (handle_command).
  4. <b>internals</b> :
   Contient les fonctions internes du shell (cd, ftype, pwd, exit).
  5. <b>utils</b>:
   Gère les boucles for sans options et implémente une méthode pour vérifier la syntaxe de for.

* ### Structures de données :

  1. <b>main</b> :
      
  2. <b>prompt</b> :
      - Variable globale : `last_status` ( valeur de retour de la dernière commande )

  3. <b>handler</b> :

  4. <b>internals</b> :
    - Utilisation de `struct stat` pour obtenir des métadonnées sur les fichiers.
    - `rep_precedent` : Stocke le chemin du répertoire précédent pour la commande cd.

  5. <b>utils</b>:
    - Des structures comme `struct dirent` et `struct stat` pour manipuler les fichiers et répertoires.

* ### Implementation des commandes locales :
   *  `my-sed` : // a completer
   *  `my-tr` : // a completer
   * `my-wlc` : // a completer

* ### Implementation des commandes internes :
   *  `exit` : quitte le shell avec un code de retour.Si un argument est donné, c'est le code de retour. Sinon on utilise last_status (le dernier statut de commande). Syntaxe : exit [VAL]

   *  `cd` : change le répertoire courant. Si y'a aucun argument on va dans le répertoire HOME. Si l’argument est "-",on revient       
             au répertoire précédent. Sinon on change vers le répertoire spécifié. Syntaxe : cd [REP]

   * `pwd` : Affiche le chemin absolu du répertoire courant. Syntaxe : pwd 

   * `ftype` : Affiche le type d’un fichier ou répertoire donné. Les types sont : “directory” ,“regular file”,“symbolic          
               link”,“namedpipe”, ou “other”. Syntaxe : ftype [FILE]


## Dépendances et Bibliothèques Externes :
Ce projet ne dépend d'aucune bibliothèque externe. Il utilise les bibliothèques standards du C.


