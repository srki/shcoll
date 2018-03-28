build:
	oshcc -c src/util/trees.c -o trees.o
	oshcc -c src/barrier.c -o barrier.o
	oshcc -c src/broadcast.c -o broadcast.o
	oshcc -c src/reduction.c -o reduction.o
	ar cr shcoll.a trees.o barrier.o broadcast.o reduction.o

clean:
	rm *.o *.a