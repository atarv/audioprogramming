CC = gcc
CFLAGS =  -g -Wall -Werror -Wextra -pedantic -std=c99
INCLUDES = -I./include -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/main.c $(SRC)/wave.c $(SRC)/breakpoints.c $(LIBS) $(INCLUDES) -o oscgen

clean:
	rm oscgen