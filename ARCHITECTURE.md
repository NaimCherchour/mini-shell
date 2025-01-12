# Projet SYS5 mini Shell fsh


## Structure du dépot :

📦mini-shell  

┣ 📂bin/ ------------------------------------> Contient les exécutables des commandes locales (`my-sed`, `my-tr`, `my-wc`).
┣ 📂obj/ -----------------------------------> Contient les fichiers objets compilés (.o) générés à partir des fichiers sources.
┣ 📂headers/ --------------------------------> Contient les fichiers d'en-tête  
┃   ┃ ┣ prompt.h
┃   ┃ ┣ internals.h
┃   ┃ ┣ handler.h
┃   ┃ ┣ utils.h 
┃   ┃ ┣ signal_handler.h 
┃   ┃ ┗ redirections.h
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
┃ ┣ main.c --------------------------------> source du fsh  ( méthode principale )
┃ ┣ signal_handler.c -----------------------> source pour la gestion des signaux 
┃ ┗ redirections.c ------------------------> code source des redirections
┣ ARCHITECTURE.md  
┣ AUTHORS.md   
┣ Makefile  
┣ README.md  
┗ fsh ------------------------------------> executable du mini shell. Géneré après compilation



## Architecture Logicielle :
* ### Description générale :
   fsh est un shell minimaliste qui supporte les fonctionnalités suivantes :
	•	Exécution de commandes internes (cd, pwd, ftype, exit) et de commandes externes
	•	Gestion des redirections d’entrée (<), sortie (>, >>, >|), et erreurs (2>, 2>>, 2>|)
	•	Gestion des commandes structurées telles que for et if-else
	•	Gestion des signaux (SIGINT, SIGTERM)
	•	Un prompt dynamique indiquant le répertoire courant et la valeur de retour de la dernière commande
* ### Principe de fonctionnement :
	1.	Lecture de la ligne de commande :
	Le shell lit une ligne entrée par l’utilisateur via `readline`.
   La ligne est analysée pour :
   - Détecter les redirections (`<`, `>`, `>>`, `2>`).
   - Identifier les commandes structurées (`for`, `if-else`).
   - Différencier les commandes internes (`cd`, `pwd`, `ftype`, `exit`) des commandes externes.

	2.	Parsing et traitement :
	•	La ligne est découpée en commandes individuelles grâce à cutout_commands.
	•	Les redirections sont détectées et extraites via detect_redirections, et leur application est effectuée par apply_redirection.
	3.	Exécution des commandes :
	•	Si une commande interne est détectée (cd, pwd, ftype, exit), elle est exécutée immédiatement.
	•	Les commandes structurées comme for ou if-else sont interprétées en fonction de leur syntaxe spécifique.
	•	Les commandes externes sont exécutées dans un processus fils via execvp.
	4.	Gestion des redirections :
	•	Avant d’exécuter une commande, les descripteurs de fichiers (stdin, stdout, stderr) sont sauvegardés.
	•	Les redirections sont appliquées en modifiant les descripteurs cibles.
	•	Après exécution, les descripteurs sont restaurés à leur état d’origine.

*### Fichiers source :
  1. **main** :
      Contient la méthode principale `main()` qui initialise le shell, appelle la génération du prompt, lit la commande entrée par l'utilisateur et délègue l'exécution des commandes.
  2. **prompt** :
      Définit la génération du prompt dynamique, incluant la gestion des couleurs, l'affichage du `last_status` et du répertoire courant avec un raccourcissement lorsque nécessaire.
  3. **handler** :
      Implémente le parsing des commandes avec `parse_command` et `cutout_commands`. Gère l'exécution des commandes internes (`cd`, `ftype`, `pwd`, `exit`) et le contrôle des processus pour les commandes externes, incluant la gestion des signaux (`handle_commands` et `execute_command`).
  4. **internals** :
      Contient les fonctions internes du shell : `cd`, `ftype`, `pwd` et `exit`.
  5. **utils** :
      Gère les boucles `for` (sans options) et inclut une méthode `for_syntaxe` pour valider la syntaxe.
  6. **redirections** :
      Implémente la gestion des redirections :
      - `detect_redirections` : Identifie les redirections (`<`, `>`, `>>`, `2>`).
      - `apply_redirection` : Applique les redirections en modifiant les descripteurs de fichiers.
      - `save_fds` / `restore_fds` : Sauvegarde et restaure les descripteurs standards (`stdin`, `stdout`, `stderr`).
  7. **signal_handler** :
      Gère les signaux (`SIGINT` et `SIGTERM`) en utilisant `sigaction` pour définir des handlers personnalisés.




* ### Structures de données :

  1. <b>main</b> :
      
  2. <b>prompt</b> :
      - Variable globale : `last_status` ( valeur de retour de la dernière commande )

  3. <b>handler</b> :
    - `char**` : représente une commande unique sous forme de tableau de chaînes, chaque élément étant un argument ou le nom de commande
    - `char ***` :  représente une séquence de commandes, chaque commande étant un tableau de chaînes , Exemple :  
    La ligne ls -l ; pwd ; echo "done" est représentée par :
    char*** commands = {
    {"ls", "-l", NULL},
    {"pwd", NULL},
    {"echo", "done", NULL},
    NULL
   };

  4. <b>internals</b> :
    - Utilisation de `struct stat` pour obtenir des métadonnées sur les fichiers.
    - `rep_precedent` : Stocke le chemin du répertoire précédent pour la commande cd.

  5. <b>utils</b>:
    - Des structures comme `struct dirent` et `struct stat` pour manipuler les fichiers et répertoires.

   6. <b>redirections</b> : 
   - `RedirectionType` (enum) qui définit les types de redirections possibles comme ( > , >> ... ). 
   - `Redirection` (struct) qui représente une redirection spécifique avec les champs : 
      - `type` ( RedirectionType ) : Type de redirection à appliquer.
	   - `file` (char*)  :Nom du fichier cible pour la redirection.

   7. <b>signal_handler </b> :
   - Utilsation du type `struct sigaction` pour définir des handlers de signaux personnalisés



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


