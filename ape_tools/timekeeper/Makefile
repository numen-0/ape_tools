CC=gcc
SRC=src
OBJ=obj
SRCS=$(wildcard $(SRC)/*.c) main.c
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
BINDIR=bin
BIN=$(BINDIR)/timekeeper
CFLAGS=-Wall

all:$(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clear:
	rm $(OBJ)/*.o $(BINDIR)/*

