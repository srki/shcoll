#include "reduction.h"

#include <time.h>
#include <shmem.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util/util.h"
#include "util/debug.h"

#define VERIFY
#define PRINTx

#define MAX(A, B) ((A) > (B) ? (A) : (B))

typedef void (*reduce_impl)(int *, const int *, int, int, int, int, int *, long *);


double test_reduce(reduce_impl reduce, int iterations, size_t count, long SYNC_VALUE, size_t REDUCE_SYNC_SIZE, size_t REDUCE_MIN_WRKDATA_SIZE) {
    long *pSync = shmem_malloc(REDUCE_SYNC_SIZE * sizeof(long));
    int *pWrk = shmem_malloc(MAX(REDUCE_MIN_WRKDATA_SIZE, count) * sizeof(int));

    for (int i = 0; i < REDUCE_SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
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
        #ifdef VERIFY
        memcpy(dst, src, count * sizeof(uint32_t));
        #endif

        shmem_barrier_all(); /* shmem_barrier_sync(); */
        reduce(dst, src, (int) count, 0, 0, shmem_n_pes(), pWrk, pSync);


        #ifdef PRINT
        if (count > 100) {
            continue;
        }

        char *print = malloc(1000000);
        char *str = print;

        for (int j = 0; j < count; j++) {
            str += sprintf(str, "%d ", dst[j]);
        }

        fprintf(stderr, "%s\n", print);

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
    shmem_free(pSync);
    shmem_free(pWrk);
    shmem_free(src);
    shmem_free(dst);
    shmem_barrier_all();

    return (end - start) / 1e9;
}


int main(int argc, char *argv[]) {
    int iterations = 10;
    size_t count;

    shmem_init();

    if (shmem_my_pe() == 0) {
        fprintf(stderr, "PEs: %d\n", shmem_n_pes());
    }

    for (count = 32; count <= 32 * 1024 * 1024; count *= 32) {
        if (shmem_my_pe() == 0) {
            gprintf("OpenSHMEM[%lu]: %lf\n", count, test_reduce(shmem_int_sum_to_all, iterations, count, SHMEM_SYNC_VALUE, SHMEM_REDUCE_SYNC_SIZE, SHMEM_REDUCE_MIN_WRKDATA_SIZE));
            gprintf("Linear[%lu]: %lf\n", count, test_reduce(shcoll_int_sum_to_all_linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE));
            gprintf("Binomial[%lu]: %lf\n", count, test_reduce(shcoll_int_sum_to_all_binomial, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE));
            gprintf("\n");
        } else {
            test_reduce(shmem_int_sum_to_all, iterations, count, SHMEM_SYNC_VALUE, SHMEM_REDUCE_SYNC_SIZE, SHMEM_REDUCE_MIN_WRKDATA_SIZE);
            test_reduce(shcoll_int_sum_to_all_linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE);
            test_reduce(shcoll_int_sum_to_all_binomial, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE);
        }
    }

    shmem_finalize();
    return 0;
}
