#include "barrier.h"
#include <stdio.h>
#include <stdlib.h>
#include "util/util.h"
#include "util/debug.h"
#include "util/run.h"

#define WARMUP

typedef void (*barrier_impl)(int, int, int, long *);

static inline void shcoll_barrier_shmem(int PE_start, int logPE_stride, int PE_size, long *pSync) {
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync);
}

double test_barrier(barrier_impl barrier, int iterations, int log2stride,
                  long SYNC_VALUE, size_t BARRIER_SYNC_SIZE) {
    long *pSync = shmem_malloc(BARRIER_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < BARRIER_SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }
    shmem_barrier_all();
    int npes = shmem_n_pes();

    #ifdef WARMUP
    for (int i = 0; i < (iterations / 10); i++) {
        barrier(0, log2stride, npes >> log2stride, pSync);
    }
    #endif

    time_ns_t start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        barrier(0, log2stride, npes >> log2stride, pSync);
    }

    shmem_barrier_all();
    time_ns_t end = current_time_ns();

    shmem_free(pSync);
    shmem_barrier_all();

    return (end - start) / 1e9;
}

int main(int argc, char **argv) {
    int iterations = (int) (argc > 1 ? strtol(argv[1], NULL, 0) : 1);
    int logPE_stride = 0;

    shmem_init();
    int me = shmem_my_pe();
    int npes = shmem_n_pes();

    if (me == 0) {
        gprintf("PEs: %d\n", npes);
    }

    RUN(barrier, shmem, iterations, logPE_stride, SHMEM_SYNC_VALUE, SHMEM_BARRIER_SYNC_SIZE);
    RUN(barrier, dissemination, iterations, logPE_stride, SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);
    RUN(barrier, binomial_tree, iterations, logPE_stride, SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);

    for (int degree = 2; degree <= 32; degree *= 2) {
        shcoll_set_tree_degree(degree);
        if (me == 0) gprintf("%2d-", degree);
        RUN(barrier, complete_tree, iterations, logPE_stride, SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);
    }

    for (int k = 2; k <= 32; k *= 2) {
        shcoll_set_knomial_tree_radix_barrier(k);
        if (me == 0) gprintf("%2d-", k);
        RUN(barrier, knomial_tree, iterations, logPE_stride, SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);
    }

    RUNC(npes <= 16 * 24, barrier, linear, iterations, logPE_stride, SHCOLL_SYNC_VALUE, SHCOLL_BARRIER_SYNC_SIZE);

    shmem_finalize();
}