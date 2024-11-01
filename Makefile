CC = gcc
CFLAGS = -Wall -g
CLIBS = -lreadline # Bibliothèque readline
EXEC = fsh


REPSRC = src
REPINC = include
REPOBJ = obj


# Liste des fichiers sources et objets
SOURCE := $(wildcard $(REPSRC)/*.c)
OBJECTS := $(patsubst $(REPSRC)/%.c,$(REPOBJ)/%.o,$(SOURCE))

INCLUDES = -I$(REPINC)

all: $(EXEC)

# Compilation de l'exécutable
$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJECTS) $(CLIBS)

# Compilation des fichiers objets
$(REPOBJ)/%.o: $(REPSRC)/%.c | $(REPOBJ)
	$(CC) $(CFLAGS) -c -o $@ $(INCLUDES) $<

# Pré-requête d'ordre; Création du répertoire pour les fichiers objets s'il n'existe pas
$(REPOBJ):
	mkdir -p $(REPOBJ)

clean:
	rm -rf $(REPOBJ)
	rm -f $(EXEC)