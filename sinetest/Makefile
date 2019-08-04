CC = gcc
CFLAGS =  -g -Wall -Werror -Wextra -pedantic
INCLUDES = -I./include -I../libportsf
LIBS = -L../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/sinetest.c $(SRC)/wave.c $(LIBS) $(INCLUDES) -o sinetest

clean:
	rm sinetest