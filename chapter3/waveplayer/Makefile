CXX = g++
CXXFLAGS =  -g -Wall -Werror -Wextra -pedantic -std=c++14
INCLUDES = -I./include -I../../libportsf
LIBS = -L../../libportsf -lm -lportaudio -lportsf
SRC = ./src

all:
	$(CXX) $(CXXFLAGS) $(SRC)/player.cpp $(LIBS) $(INCLUDES) -o player

clean:
	rm hellotable