build:
	oshcc -c src/util/trees.c -o trees.o
	oshcc -c src/barrier.c -o barrier.o
	oshcc -c src/broadcast.c -o broadcast.o
	ar cr collectives.a trees.o barrier.o broadcast.o

clean:
	rm *.o *.a