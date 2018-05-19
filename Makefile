CC=oshcc -g


all: build test

build: shcoll.a

shcoll.a: trees.o rotate.o barrier.o broadcast.o reduction.o collect.o fcollect.o
	ar cr shcoll.a $^

trees.o: src/util/trees.c
	$(CC) -c $< -o $@

rotate.o: src/util/rotate.c
	$(CC) -c $< -o $@

barrier.o: src/barrier.c
	$(CC) -c $< -o $@

broadcast.o: src/broadcast.c
	$(CC) -c $< -o $@

reduction.o: src/reduction.c
	$(CC) -c $< -o $@

collect.o: src/collect.c
	$(CC) -c $< -o $@

fcollect.o: src/fcollect.c
	$(CC) -c $< -o $@

test: barrier.test broadcast.test reduction.test collect.test fcollect.test

barrier.test: test/barrier_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@

broadcast.test: test/broadcast_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@

reduction.test: test/reduction_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@

collect.test: test/collect_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@

fcollect.test: test/fcollect_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@


clean:
	rm -f *.o *.a *.test
