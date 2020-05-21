# Audio programming

This repository contains my solutions to some of the exercises in Boulanger
et al.'s 
[The Audio Programming Book](https://mitpress.mit.edu/books/audio-programming-book).

## Compiling and running

To compile programs of this repository, change to directory containing a
Makefile. Then run command `make`. If everything goes allright you should get
an executable named after the directory you are in. The programs will show
required parameters, when run without any arguments.

In chapter 2 programs are compiled with `gcc`, though you could modify the
Makefiles to use any C-compiler that supports C99.

In chapter 3 programs are compiled with `g++` (C++14 and upwards). You need
to have [portaudio](http://portaudio.com/) installed as the programs depend
on it.

Chapter 6 is also in C++ and the programs depend on
[libsndfile](http://www.mega-nerd.com/libsndfile/).
