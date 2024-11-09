# Projet SYS5 mini Shell fsh


## Structure du dépot :

📦mini-shell  
 ┣ 📂bin ------------------------------------> Contient les binaires executables pour les commandes internes. Géneré apres compilation  
 ┃ ┣ 📜sed   
 ┃ ┣ 📜tr  
 ┃ ┗ 📜wlc  
 ┣ 📂src ------------------------------------> Contient les sources   
 ┃ ┣ 📂builtins  ----------------------------> Contient les sources des commandes internes  
 ┃ ┃ ┣ 📜sed.c  
 ┃ ┃ ┣ 📜tr.c  
 ┃ ┃ ┗ 📜wlc.c  
 ┃ ┗ 📜main.c ------------------------------> source du fsh  
 ┣ 📜.gitignore  
 ┣ 📜ARCHITECTURE.md  
 ┣ 📜AUTHORS.md  
 ┣ 📜Makefile  
 ┣ 📜README.md  
 ┗ 📜fsh ------------------------------------> executable du mini shell. Géneré apres compilation



## Architecture Logicielle :
* ### Description générale :
   Le mini shell fsh est un shell minimaliste qui permet d'exécuter des commandes internes et externes. Il est capable de gérer les commandes internes suivantes : `sed`, `tr`, `wlc`. Il permet également de gérer les commandes externes en utilisant la fonction `execvp`. Le mini shell fsh est capable de gérer les redirections d'entrée et de sortie, les pipes et les signaux.

* ### Principe de foncitonnement :
   1. Le mini shell fsh lit une ligne de commande entrée par l'utilisateur.
   2. Il parse ensuite la ligne de commande avec la fonction `parse_command` pour extraire les arguments et les options.
   3. Traiter la commande avec la fonction `handle_command`. <u>ie</u>:  Il vérifie s'il s'agit d'une commande interne ou externe en procedant ainsi:
      * <b>Priorité aux internes:</b> Recherche si un executable dans le répertoire `bin` portant le meme nom que la commande existe. Si oui, il s'agit d'une commande interne. 
      * Le cas echeant ,c'est une commande externe, il crée un processus fils pour exécuter la commande externe.



* ### Implementation des commandes internes :
   1. **sed, wlc et tr** : Vues au TP3

   2. **exit** : // a completer
   3. **cd** : // a completer
   4. **pwd** : // a completer
   5. **ls** : // a completer


## Dépendances et Bibliothèques Externes :
Ce projet ne dépend d'aucune bibliothèque externe. Il utilise les bibliothèques standards du C.


