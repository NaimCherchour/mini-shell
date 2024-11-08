CC = gcc
CFLAGS = -Wall -g
CLIBS = -lreadline # Bibliothèque readline
EXEC = fsh

REPSRC = src
REPINC = include
REPOBJ = obj
REPBIN = bin

# Liste des fichiers sources et objets
SOURCE := $(wildcard $(REPSRC)/*.c)
OBJECTS := $(patsubst $(REPSRC)/%.c,$(REPOBJ)/%.o,$(SOURCE))

# Builtins sources and objects
BUILTINS_SRC := $(wildcard $(REPSRC)/builtins/*.c)
BUILTINS_OBJ := $(patsubst $(REPSRC)/builtins/%.c,$(REPOBJ)/%.o,$(BUILTINS_SRC))
BUILTINS_EXEC := $(patsubst $(REPSRC)/builtins/%.c,$(REPBIN)/%,$(BUILTINS_SRC))

INCLUDES = -I$(REPINC)

all: $(EXEC) $(BUILTINS_EXEC)

# Compilation de l'exécutable principal
$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJECTS) $(CLIBS)

# Compilation des fichiers objets
$(REPOBJ)/%.o: $(REPSRC)/%.c | $(REPOBJ)
	$(CC) $(CFLAGS) -c -o $@ $(INCLUDES) $<

# Compilation des fichiers objets pour les builtins
$(REPOBJ)/%.o: $(REPSRC)/builtins/%.c | $(REPOBJ)
	$(CC) $(CFLAGS) -c -o $@ $(INCLUDES) $<

# Linking des exécutables pour les builtins
$(REPBIN)/%: $(REPOBJ)/%.o | $(REPBIN)
	$(CC) $(CFLAGS) -o $@ $< $(CLIBS)

# Pré-requête d'ordre; Création des répertoires pour les fichiers objets et binaires s'ils n'existent pas
$(REPOBJ):
	mkdir -p $(REPOBJ)

$(REPBIN):
	mkdir -p $(REPBIN)

clean:
	rm -rf $(REPOBJ)
	rm -f $(EXEC)
	rm -f $(REPBIN)/*

run: $(EXEC)
	@clear
	@./$(EXEC)