CC = clang
CFLAGS = -g -Wall -Werror -Wextra -pedantic -I./include -I../libportsf
LIBS = -L../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/sfnorm.c $(LIBS) -o sfnorm

clean:
	rm sf2float