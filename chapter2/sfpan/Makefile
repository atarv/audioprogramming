CC = gcc
CFLAGS =  -g -Wall -Werror -Wextra -pedantic
INCLUDES = -I./include -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/sfpan.c $(SRC)/breakpoints.c $(LIBS) $(INCLUDES) -o sfpan

clean:
	rm sfpan