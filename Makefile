CC = gcc

CFLAGS = -Wall $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf -lm

SRC = game.c

TARGET = output-directory/main-game

# Default target
all: clean $(TARGET)

$(TARGET): $(SRC)
	mkdir output-directory
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	cp -r images output-directory/
	cp COMIC.TTF output-directory/

# Clean up build files
clean:
	rm -rf output-directory

# Phony targets
.PHONY: all clean
