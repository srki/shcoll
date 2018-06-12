CC_FLAGS=-g -O2
CC=cc $(CC_FLAGS)
#CC=$(HOME)/libs/SOS/_install/bin/oshcc $(CC_FLAGS)
#CC=$(HOME)/libs/SOS-1.4.1/_install/bin/oshcc -std=c99 -D_POSIX_C_SOURCE=199309L $(CC_FLAGS)

all: build test


build: shcoll.a

shcoll.a: bithacks.o trees.o rotate.o scan.o barrier.o broadcast.o reduction.o collect.o fcollect.o alltoall.o alltoalls.o broadcast-size.o
	ar cr shcoll.a $^

bithacks.o: src/util/bithacks.c
	$(CC) -c $< -o $@

trees.o: src/util/trees.c
	$(CC) -c $< -o $@

rotate.o: src/util/rotate.c
	$(CC) -c $< -o $@

scan.o: src/util/scan.c
	$(CC) -c $< -o $@

broadcast-size.o: src/util/broadcast-size.c
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

alltoall.o: src/alltoall.c
	$(CC) -c $< -o $@

alltoalls.o: src/alltoalls.c
	$(CC) -c $< -o $@


test: barrier.test broadcast.test reduction.test collect.test fcollect.test alltoall.test alltoalls.test

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

alltoall.test: test/alltoall_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@

alltoalls.test: test/alltoalls_test.c shcoll.a
	$(CC) $< shcoll.a -Isrc -o $@


clean:
	rm -f *.o *.a *.test
