PKGS=sdl2 glew glu gl libpng16 bullet
CC=clang
CCCP=clang++
OPT=-Ofast
#OPT=-O0 -g
CFLAGS=-Wall $(OPT) $(shell pkg-config $(PKGS) --cflags)
CCFLAGS=--std=c++11 -Woverloaded-virtual $(CFLAGS)
LINK=-lm $(shell pkg-config $(PKGS) --libs)

all: main

sim.o: sim.cc
	$(CCCP) $(CCFLAGS) -c sim.cc

a.o: a.c a.h
	$(CC) $(CFLAGS) -c a.c

m.o: m.c m.h
	$(CC) $(CFLAGS) -c m.c

d.o: d.c d.h
	$(CC) $(CFLAGS) -c d.c

shader.o: shader.c shader.h
	$(CC) $(CFLAGS) -c shader.c

render.o: render.c render.h magic.h
	$(CC) $(CFLAGS) -c render.c

track.o: track.c track.h
	$(CC) $(CFLAGS) -c track.c

editor.o: editor.c editor.h
	$(CC) $(CFLAGS) -c editor.c

game.o: game.c game.h magic.h
	$(CC) $(CFLAGS) -c game.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

main: main.o sim.o a.o m.o d.o shader.o render.o track.o editor.o game.o
	$(CCCP) main.o sim.o a.o m.o d.o shader.o render.o track.o editor.o game.o -o main $(LINK)

clean:
	rm -f *.o main
