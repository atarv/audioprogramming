CXX = g++
CXXFLAGS =  -g -Wall -Werror -Wextra -pedantic -std=c++14
INCLUDES = -I./include
LIBS = -lm -lsndfile
SRC = ./src

all:
	$(CXX) $(CXXFLAGS) $(SRC)/interposc.cpp $(LIBS) $(INCLUDES) -o interposc

clean:
	rm hellotable