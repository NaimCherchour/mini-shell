# Projet SYS5 mini Shell fsh


## Structure du dépot :

📦mini-shell  
 ┣ 📂bin/ ------------------------------------> Contient les binaires executables pour les commandes locales.  

┣ 📂obj/ -----------------------------------> Contient les fichiers .o┃
┣ 📂headers/ --------------------------------> Contient les fichiers d'en-tête  
   ┃ ┣ prompt.h
   ┃ ┣ internals.h
 ┣ 📂src/ ------------------------------------> Contient les sources   
 ┃ ┣ 📂locals/  ----------------------------> Contient les sources des commandes locales  
 ┃ ┃ ┣ my-sed.c  
 ┃ ┃ ┣ my-tr.c  
 ┃ ┃ ┗ my-wlc.c  
 ┃  ┃ prompt.c ------------------------------> source de la fonction d'affichage du prompt  
 ┃ ┣ internals.c ---------------------------> source des commandes internes
 ┃ ┗ main.c ------------------------------> source du fsh  
 ┣ ARCHITECTURE.md  
 ┣ AUTHORS.md  
 ┣ Makefile  
 ┣ README.md  
 ┗ fsh ------------------------------------> executable du mini shell. Géneré apres compilation



## Architecture Logicielle :
* ### Description générale :
   Le mini shell fsh est un shell minimaliste qui permet d'exécuter des commandes internes et externes. Il est capable de gérer les commandes internes suivantes : `sed`, `tr`, `wlc`. Il permet également de gérer les commandes externes en utilisant la fonction `execvp`. Le mini shell fsh est capable de gérer les redirections d'entrée et de sortie, les pipes et les signaux.

* ### Principe de foncitonnement :
   1. Le mini shell fsh lit une ligne de commande entrée par l'utilisateur.
   2. Il parse ensuite la ligne de commande avec la fonction `parse_command` pour extraire les arguments et les options.
   3. Traiter la commande avec la fonction `handle_command`. <u>ie</u>:  Il vérifie s'il s'agit d'une commande interne ou externe en procedant ainsi:
      1. <b>Priorité aux internes:</b> 
      2. <b>Recherche de commandes locales: </b>Recherche si un executable dans le répertoire `bin` portant le meme nom que la commande existe. Si oui, il s'agit d'une commande interne. Il crée un processus fils pour exécuter la commande locale.
      3. <b>Enfin:</b> si on trouve rien, c'est une commande externe. il crée un processus fils pour exécuter la commande externe.



* ### Implementation des commandes locales :
   *  `my-sed` : // a completer
   *  `my-tr` : // a completer
   * `my-wlc` : // a completer

* ### Implementation des commandes internes :
   *  `exit` : // a completer
   *  `cd` : // a completer
   * `pwd` : // a completer
   * `ftype` : // a completer


## Dépendances et Bibliothèques Externes :
Ce projet ne dépend d'aucune bibliothèque externe. Il utilise les bibliothèques standards du C.


