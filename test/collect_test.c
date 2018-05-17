//
// Created by Srdan Milakovic on 5/15/18.
//

#include "collect.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

#define VERIFY
#define PRINTx

#define gprintf(...) fprintf(stderr, __VA_ARGS__)


typedef void (*collect_impl)(void *, const void *, size_t, int, int, int, long *);

double test_collect(collect_impl collect, int iterations, size_t count) {
    #ifdef PRINT
    long *lock = shmem_malloc(sizeof(long));
    *lock = 0;
    #endif

    long *pSync = shmem_malloc(SHMEM_COLLECT_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < SHMEM_COLLECT_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    shmem_barrier_all();

    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    size_t nelems = count * (me + 1);

    size_t total_size = (size_t) (count * (1 + npes) * npes / 2);

    uint32_t *dst = shmem_malloc(total_size * sizeof(uint32_t));
    uint32_t *src = shmem_malloc(count * npes * sizeof(uint32_t));

    for (int i = 0; i < nelems; i++) {
        src[i] = (uint32_t) ((me + 1) * (i + 1));
    }

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memcpy(dst, src, total_size * sizeof(uint32_t));
        #endif

        collect(dst, src, nelems, 0, 0, npes, pSync);
        shmem_sync_all();

        #ifdef PRINT
        shmem_set_lock(lock);
        printf("%d: %d", shmem_my_pe(), dst[0]);
        for (int j = 1; j < total_size; j++) { printf(", %d", dst[j]); }
        printf("\n");
        shmem_clear_lock(lock);
        #endif

        #ifdef VERIFY
        int j = 0;
        for (int p = 0; p < npes; p++) {
            for (int k = 0; k < (p + 1) * count; k++, j++) {
                if (dst[j] != (p + 1) * (k + 1)) {
                    gprintf("[%d] %d %d\n", shmem_my_pe(), dst[j], (p + 1) * (k + 1));
                    abort();
                }
            }
        }
        #endif
    }

    shmem_barrier_all();
    unsigned long long end = current_time_ns();

    #ifdef PRINT
    shmem_free(lock);
    #endif

    shmem_free(pSync);
    shmem_free(src);
    shmem_free(dst);

    return (end - start) / 1e9;

}


int main(int argc, char *argv[]) {
    int iterations = 1;
    size_t count = 1;

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf("[%s]PEs: %d\n", __FILE__, shmem_n_pes());
    }

    if (shmem_my_pe() == 0) {
        gprintf("shmem: %lf\n", test_collect(shmem_collect32, iterations, count));
        gprintf("linear: %lf\n", test_collect(shcoll_collect32_linear, iterations, count));
    } else {
        test_collect(shmem_collect32, iterations, count);
        test_collect(shcoll_collect32_linear, iterations, count);
    }

    shmem_finalize();
}