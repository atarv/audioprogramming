CC = g++
CFLAGS = -O2 -Wall -I/usr/include
PSFOBJS = tinyAudioLib.o
LIBS = -lm -lportaudio

.c.o:
	$(CC) $(CFLAGS) -c $<

all:	libtiny.a

libtiny.a:	$(PSFOBJS)
	ar rc libtiny.a $(PSFOBJS)

tinyAudioLib.o:	./tinyAudioLib.h 
	$(CC) $(CFLAGS) tinyAudioLib.c $(LIBS) -c


