CC = gcc
CFLAGS = -g -Wall -Werror -Wextra -pedantic -I./include -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src
INCLUDE = ./include


all:
	$(CC) $(CFLAGS) $(SRC)/sf2float.c $(SRC)/sfprop.c $(LIBS) -o sf2float

clean:
	rm sf2float