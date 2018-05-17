#include "barrier.h"
#include <stdio.h>
#include "util/util.h"
#include "util/debug.h"

typedef void (*barrier_impl)(int, int, int, long *);

void test(barrier_impl barrier, int iterations, int log2stride, char *name, long SYNC_VALUE, size_t BARRIER_SYNC_SIZE) {
    long *pSync = shmem_malloc(BARRIER_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < BARRIER_SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }
    shmem_barrier_all();

    unsigned long long start = current_time_ns();
    for (int i = 0; i < iterations; i++) {
        barrier(0, log2stride, shmem_n_pes() >> log2stride, pSync);
    }

    if (shmem_my_pe() == 0) {
        gprintf("%s: %lf\n", name, (current_time_ns() - start) / 1e9);
    }

    shmem_free(pSync);
    shmem_barrier_all();
}

int main(int argc, char **argv) {
    int iterations = 25000;

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf("PEs: %d\n", shmem_n_pes());
    }

    test(shmem_barrier, iterations, 0, "shmem", SHMEM_SYNC_VALUE, SHMEM_BARRIER_SYNC_SIZE);
    test(shcoll_linear_barrier, iterations, 0, "Linear", SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);
    test(shcoll_dissemination_barrier, iterations, 0, "Dissemination", SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);
    test(shcoll_binomial_tree_barrier, iterations, 0, "Binomial", SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);

    for (int degree = 2; degree < 64; degree *= 2) {
        char name[50];
        sprintf(name, "Complete - %d", degree);
        shcoll_set_tree_degree(degree);
        test(shcoll_complete_tree_barrier, iterations, 0, name, SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);
    }

    shmem_finalize();
    return 0;
}