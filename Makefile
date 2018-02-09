build:
	oshcc -c src/util/trees.c -o trees.o
	oshcc -c src/barrier.c -o barrier.o
	ar cr collectives.a trees.o barrier.o