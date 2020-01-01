CC = gcc
CFLAGS =  -g -Wall -Werror -Wextra -pedantic
INCLUDES = -I./include -I../../libportsf
LIBS = -L../../libportsf -lportsf -lm
SRC = ./src

all:
	$(CC) $(CFLAGS) $(SRC)/envx.c $(LIBS) $(INCLUDES) -o envx

clean:
	rm envx