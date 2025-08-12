# Makefile for Raylib Match-3 project (run from project root)

CC = gcc
CFLAGS = -Wall -std=c99
LDFLAGS = -lraylib -lm -ldl -lpthread -lGL -lX11

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/main
SRC = $(SRC_DIR)/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -rf $(BIN_DIR)
