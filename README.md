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

> [!TIP]
> To see other available target, use `make help`

---

## Features

- [x] External commands
- [x] Local commands
- [x] Internal commands 
- [x] Commands history
- [ ] Special characters
- [ ] Piping
- [ ] Redirections
- [ ] Signals
- [ ] command structuring
- [ ] `for` loop
- [ ] `if else` conditional branching
### Shortcuts
- [x] UP and DOWN arrows (seeking command history)
- [x] LEFT and RIGHT arrows (moving the current prompt cursor)

### Builtins
- [x] `exit`
- [x] `cd`
- [ ] `echo`
- [x] `pwd`
- [ ] `ls`
- [x] `wlc` aka `wc`
- [x] `tr`
- [x] `sed` 
