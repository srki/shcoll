all: build test

build:
	oshcc -c src/util/trees.c -o trees.o
	oshcc -c src/barrier.c -o barrier.o
	oshcc -c src/broadcast.c -o broadcast.o
	oshcc -c src/reduction.c -o reduction.o
	ar cr shcoll.a trees.o barrier.o broadcast.o reduction.o

test: barrier broadcast reduction

barrier: test/barrier_test.c build
	oshcc $< shcoll.a -Isrc -o $@

broadcast: test/broadcast_test.c build
	oshcc $< shcoll.a -Isrc -o $@

reduction: test/reduction_test.c build
	oshcc $< shcoll.a -Isrc -o $@

clean:
	rm barrier broadcast reduction *.o *.a
