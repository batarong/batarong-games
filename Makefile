CC = gcc

SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null || sdl2-config --cflags)
SDL2_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null || sdl2-config --libs)
SDL2_TTF_LIBS := $(shell pkg-config --libs SDL2_ttf 2>/dev/null || echo -lSDL2_ttf)

CFLAGS ?= -O2 -Wall
CFLAGS += $(SDL2_CFLAGS)
LDFLAGS += $(SDL2_LIBS) $(SDL2_TTF_LIBS) -lm

SRC = game.c
TARGET_DIR = output-directory
TARGET = $(TARGET_DIR)/main-game

all: $(TARGET)

$(TARGET): $(SRC) | $(TARGET_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)
	cp -r images $(TARGET_DIR)/
	cp COMIC.TTF $(TARGET_DIR)/

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g -O0
debug: clean all

clean:
	rm -rf $(TARGET_DIR)

.PHONY: all clean run debug
