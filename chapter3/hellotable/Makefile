CXX = g++
CXXFLAGS =  -g -Wall -Werror -Wextra -pedantic -std=c++14
INCLUDES = -I./include
LIBS = -lm -lportaudio
SRC = ./src

all:
	$(CXX) $(CXXFLAGS) $(SRC)/main.cpp $(LIBS) $(INCLUDES) -o hellotable

clean:
	rm hellotable