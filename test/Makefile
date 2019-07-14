CC = clang
CFLAGS = -g -Wall -Werror -Wextra -I../libportsf
LIBS = -L../libportsf -lportsf -lm
SRC = ./src


all:
	$(CC) $(CFLAGS) $(SRC)/main.c $(LIBS)

clean:
	rm a.out