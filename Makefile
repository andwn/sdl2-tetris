CC?=gcc
#CC=i686-w64-mingw32-gcc
#CC=clang

SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)

CFLAGS=-std=c99 -O2 -Wall
LIBS=-lSDL2 -lSDL2_ttf
OUTPUT=tetris

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTPUT) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	
clean:
	rm *.o
	rm $(OUTPUT)

package:
	tar cfv sdl2-tetris.tar $(OUTPUT) data/*
