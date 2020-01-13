CC = gcc
CFLAGS =  -g -Wall -Werror -Wextra -pedantic -std=c99
INCLUDES = -I./include -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/main.c $(SRC)/wave.c $(SRC)/breakpoints.c $(SRC)/gtable.c $(LIBS) $(INCLUDES) -o tabgen

clean:
	rm oscgen