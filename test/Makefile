all: barrier broadcast reduction


barrier: barrier_test.c
	oshcc barrier_test.c ../shcoll.a -I../src -o $@

broadcast: broadcast_test.c
	oshcc broadcast_test.c ../shcoll.a -I../src -o $@

reduction: reduction_test.c
	oshcc reduction_test.c ../shcoll.a -I../src -o $@

clean:
	rm barrier broadcast reduction