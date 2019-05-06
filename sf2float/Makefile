CC = clang
CFLAGS = -g -Wall -Werror -Wextra -pedantic -I./include
LIBS = -L./lib -lportsf -lm
SRC = ./src
LIB = ./lib
INCLUDE = ./include


all:
	$(CC) $(CFLAGS) $(SRC)/sf2float.c $(SRC)/sfprop.c $(LIBS) -o sf2float