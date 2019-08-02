CC = clang
CFLAGS =  -Wall -Werror -Wextra -pedantic -O3
INCLUDES = -I./include -I../libportsf
LIBS = -L../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/sfpan.c $(SRC)/breakpoints.c $(LIBS) $(INCLUDES) -o sfpan

clean:
	rm sf2float