//
// Created by Srdan Milakovic on 5/17/18.
//


#include "fcollect.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <collect.h>
#include "util/util.h"
#include "util/debug.h"

#define PRINTx
#define VERIFY

typedef void (*fcollect_impl)(void *, const void *, size_t, int, int, int, long *);

double test_fcollect(fcollect_impl fcollect, int iterations, size_t nelem, long SYNC_VALUE, size_t COLLECT_SYNC_SIZE) {
    #ifdef PRINT
    long *lock = shmem_malloc(sizeof(long));
    *lock = 0;
    #endif

    long *pSync = shmem_malloc(COLLECT_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < COLLECT_SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }

    shmem_barrier_all();

    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    size_t total_nelem = nelem * npes;

    uint32_t *src = shmem_malloc(nelem * sizeof(uint32_t));
    uint32_t *dst = shmem_malloc(total_nelem * sizeof(uint32_t));

    for (int i = 0; i < nelem; i++) {
        src[i] = (uint32_t) (total_nelem - me * nelem - i);
    }

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memset(dst, 0, total_nelem * sizeof(uint32_t));
        #endif

        shmem_barrier_all(); /* shmem_sync_all(); */

        fcollect(dst, src, nelem, 0, 0, npes, pSync);

        #ifdef PRINT
        shmem_set_lock(lock);
        printf("%d: %d", shmem_my_pe(), dst[0]);
        for (int j = 1; j < total_nelem; j++) { printf(", %d", dst[j]); }
        printf("\n");
        shmem_clear_lock(lock);
        #endif

        #ifdef VERIFY
        for (int j = 0; j < total_nelem; j++) {
            if (total_nelem - j != dst[j]) {
                gprintf("i:%d [%d] dst[%d] = %d; Expected %zu\n", i, shmem_my_pe(), j, dst[j], total_nelem - j);
                abort();
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
    int iterations = 1000;
    size_t count = 10000;

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf("[%s]PEs: %d\n", __FILE__, shmem_n_pes());
    }

    if (shmem_my_pe() == 0) {
        gprintf("shmem: %lf\n", test_fcollect(shmem_fcollect32, iterations, count, SHMEM_SYNC_VALUE, SHMEM_COLLECT_SYNC_SIZE));
        gprintf("linear: %lf\n", test_fcollect(shcoll_fcollect32_linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE));
    } else {
        test_fcollect(shmem_fcollect32, iterations, count, SHMEM_SYNC_VALUE, SHMEM_COLLECT_SYNC_SIZE);
        test_fcollect(shcoll_fcollect32_linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
    }

    shmem_finalize();
}