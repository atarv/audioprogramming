CC = gcc
CFLAGS =  -g -Wall -Werror -Wextra -pedantic
INCLUDES = -I./include -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/sfenv.c $(SRC)/breakpoints.c $(LIBS) $(INCLUDES) -o sfenv

clean:
	rm sfenv