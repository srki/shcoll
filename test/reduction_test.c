#include "reduction.h"

#include <time.h>
#include <shmem.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define VERIFY

#define MAX(A, B) ((A) > (B) ? (A) : (B))

unsigned long long current_time_ns() {
    struct timespec t = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    unsigned long long s = 1000000000ULL * (unsigned long long) t.tv_sec;
    return (((unsigned long long) t.tv_nsec)) + s;
}

typedef void (*reduce_impl)(int *, const int *, int, int, int, int, int *, long *);


double test_reduce(reduce_impl reduce, int iterations, size_t count) {
    long *pSyc = shmem_malloc(SHMEM_REDUCE_SYNC_SIZE * sizeof(long));
    int *pWrk = shmem_malloc(MAX(SHMEM_REDUCE_MIN_WRKDATA_SIZE, count) * sizeof(int));

    for (int i = 0; i < SHMEM_REDUCE_MIN_WRKDATA_SIZE; i++) {
        pSyc[i] = SHMEM_SYNC_VALUE;
    }

    shmem_barrier_all();

    int *dst = shmem_calloc(count, sizeof(int));
    int *src = shmem_calloc(count, sizeof(int));

    for (int i = 0; i < count; i++) {
        src[i] = (i % 10007) * shmem_my_pe();
    }

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        reduce(dst, src, (int) count, 0, 0, shmem_n_pes(), pWrk, pSyc);

        shmem_sync_all();

        #ifdef PRINT
        if (count > 100) {
            continue;
        }

        char *print = malloc(1000000);
        char *str = print;

        for (int j = 0; j < count; j++) {
            str += sprintf(str, "%d ", dst[j]);
        }

        printf("%s\n", print);

        free(print);
        #endif

        #ifdef VERIFY
        int sum = (shmem_n_pes() - 1) * shmem_n_pes() / 2;
        for (int j = 0; j < count; j++) {
            assert(dst[j] == sum * (j % 10007));

        }

        #endif
    }


    unsigned long long end = current_time_ns();

    shmem_barrier_all();
    shmem_free(pSyc);
    shmem_free(pWrk);
    shmem_barrier_all();

    return (end - start) / 1e9;
}

int main(int argc, char *argv) {
    shmem_init();

    //test_reduce(shmem_int_sum_to_all, 1, 10);

    test_reduce(int_sum_to_all_helper_linear, 1, 100);

    shmem_finalize();
    return 0;
}
