
CC = gcc
FLAGS = -Wall
SRC = src
BIN = bin

all: client server

client: $(SRC)/client.c
	$(CC) $(FLAGS) -o $@ $< $(FLAGS)

server: $(SRC)/server.c
	$(CC) $(FLAGS) -o $@ $< $(FLAGS)

clean:
	rm -f $(wildcard bin/*.o)
