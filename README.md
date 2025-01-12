# fsh
Shell written in C. The goal is to make a mini shell handling for loops, if condition jumps, commands piping and offer some builtin commands.

## Quick Start

* Clone the repo
```sh
git clone https://moule.informatique.univ-paris-diderot.fr/benmezia/mini-shell
cd mini-shell
```

* Build the project
```sh
make
```
* Run the shell!
```sh
./fsh
```
or:
```sh
make run
```

* Clean the project 
```sh
make clean
```

> [!TIP]
> To see other available target, use `make help`

---

## Gestion des erreurs
- Le shell affiche des messages d'erreur clairs lors de l'exécution de commandes invalides ou en cas de problème de mémoire.
- La gestion des signaux est implémentée pour ignorer `SIGINT` et `SIGTERM`.
- Les commandes locales (`my-sed`, `my-tr`, `my-wc`) gèrent les erreurs de manière explicite (options invalides, fichiers inexistants).

## Features

- [x] External commands
- [x] Local commands
- [x] Internal commands 
- [x] Commands history
- [x] Special characters
- [x] Piping
- [x] Redirections
- [x] Signals
- [x] command structuring
- [x] `for` loop
- [x] `if else` conditional branching
### Shortcuts
- [x] UP and DOWN arrows (seeking command history)
- [x] LEFT and RIGHT arrows (moving the current prompt cursor)

### Builtins
- [x] `exit`
- [x] `cd`
- [x] `pwd`
- [x] `ftype`
- [x] `wlc` aka `wc`
- [x] `tr`
- [x] `sed` 
