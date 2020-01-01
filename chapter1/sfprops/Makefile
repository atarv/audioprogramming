CC = gcc
CFLAGS = -g -Wall -Werror -Wextra -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src


all:
	$(CC) $(CFLAGS) $(SRC)/sfprops.c $(LIBS) -o sfprops

clean:
	rm a.out
