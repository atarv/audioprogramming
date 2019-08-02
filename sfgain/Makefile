CC = gcc
CFLAGS = -g -Wall -Werror -Wextra -pedantic -I./include -I../libportsf
LIBS = -L../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/sfgain.c $(LIBS) -o sfgain

clean:
	rm sfgain