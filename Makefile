CC=gcc
CFLAGS=-lm -lasound -lSDL2

all:
	$(CC) alsascope.c -o alsascope $(CFLAGS)
