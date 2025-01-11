# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -I/usr/include/SDL2 -lSDL2_ttf -lSDL2

# Source file
SRC = test.c

# Executable name
TARGET = output-directory/test

# Default target
all: clean $(TARGET)

# Rule to compile the source file into the executable
$(TARGET): $(SRC)
	mkdir output-directory
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	cp -r images output-directory/
	cp COMIC.TTF output-directory/

# Clean up build files
clean:
	rm -rf output-directory

# Phony targets
.PHONY: all clean
