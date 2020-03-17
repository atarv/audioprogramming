CXX = g++
CXXFLAGS =  -g -Wall -Werror -Wextra -pedantic
INCLUDES = -I./include -I../../TinyAudioLibrary
LIBS = -lm -L../../TinyAudioLibrary -ltiny -lportaudio
SRC = ./src

all:
	$(CXX) $(CXXFLAGS) $(SRC)/main.cpp $(LIBS) $(INCLUDES) -o hellotable

clean:
	rm hellotable