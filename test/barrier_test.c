#include "barrier.h"
#include <time.h>
#include <stdio.h>

unsigned long long current_time_ns() {
    struct timespec t = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    unsigned long long s = 1000000000ULL * (unsigned long long) t.tv_sec;
    return (((unsigned long long) t.tv_nsec)) + s;
}

typedef void (*barrier_impl)(int, int, int, long *);

void test(barrier_impl barrier, int iterations, int log2stride, char *name) {
    long *pSync = shmem_malloc(SHMEM_BARRIER_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < SHMEM_BARRIER_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }
    shmem_barrier_all();

    unsigned long long start = current_time_ns();
    for (int i = 0; i < iterations; i++) {
        barrier(0, log2stride, shmem_n_pes() >> log2stride, pSync);
    }

    if (shmem_my_pe() == 0) {
        printf("%s: %lf\n", name, (current_time_ns() - start) / 1e9);
    }

    shmem_free(pSync);
    shmem_barrier_all();
}

int main(int argc, char *argv) {
    int iterations = 25000;

    shmem_init();

    if (shmem_my_pe() == 0) {
        printf("PEs: %d\n", shmem_n_pes());
    }

    test(shmem_barrier, iterations, 0, "shmem");
    test(shcoll_linear_barrier, iterations, 0, "Linear");
    test(shcoll_dissemination_barrier, iterations, 0, "Dissemination");
    test(shcoll_binomial_tree_barrier, iterations, 0, "Binomial");

    for (int degree = 2; degree < 64; degree *= 2) {
        char name[50];
        sprintf(name, "Complete - %d", degree);
        shcoll_set_tree_degree(degree);
        test(shcoll_complete_tree_barrier, iterations, 0, name);
    }


    shmem_finalize();
    return 0;
}