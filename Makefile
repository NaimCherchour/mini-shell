CC = gcc
CFLAGS = -Wall
CLIBS = -lreadline # Bibliothèque readline
EXEC = fsh

REPSRC = src
REPINC = include
REPBIN = bin

# Builtins sources and objects
BUILTINS_SRC := $(wildcard $(REPSRC)/builtins/*.c)
BUILTINS_OBJ := $(patsubst $(REPSRC)/builtins/%.c,$(REPBIN)/%.o,$(BUILTINS_SRC))

INCLUDES = -I$(REPINC)

all: $(EXEC) $(BUILTINS_OBJ)

# Compilation de l'exécutable principal
$(EXEC): $(REPSRC)/main.c $(BUILTINS_OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(REPSRC)/main.c $(BUILTINS_OBJ) $(CLIBS) $(INCLUDES)

# Compilation des fichiers objets pour les builtins
$(REPBIN)/%.o: $(REPSRC)/builtins/%.c | $(REPBIN)
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDES)

# Pré-requête d'ordre; Création des répertoires pour les fichiers binaires s'ils n'existent pas
$(REPBIN):
	mkdir -p $(REPBIN)

clean:
	rm -f $(EXEC)
	rm -f $(REPBIN)/*

run: $(EXEC)
	@clear
	@./$(EXEC)