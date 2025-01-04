# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -I/usr/include/SDL2

# Source file
SRC = test.c

# Executable name
TARGET = output-directory/test

# Default target
all: clean $(TARGET)

# Rule to compile the source file into the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -lSDL2
	cp -r images output-directory/

# Clean up build files
clean:
	rm -rf output-directory/*

# Phony targets
.PHONY: all clean
