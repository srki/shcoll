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
#include "util/run.h"

#define VERIFYx
#define PRINTx
#define WARMUP

typedef void (*fcollect_impl)(void *, const void *, size_t, int, int, int, long *);

static inline void shcoll_fcollect32_shmem(void *dest, const void *source, size_t nelems, int PE_start,
                                           int logPE_stride, int PE_size, long *pSync) {
    shmem_fcollect32(dest, source, nelems, PE_start, logPE_stride, PE_size, pSync);
}

double test_fcollect32(fcollect_impl fcollect, int iterations, size_t nelem,
                       long SYNC_VALUE, size_t COLLECT_SYNC_SIZE) {
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


    #ifdef WARMUP
    shmem_barrier_all();

    for (int i = 0; i < iterations / 10; i++) {
        shmem_barrier_all();
        fcollect(dst, src, nelem, 0, 0, npes, pSync);
    }
    #endif

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memset(dst, 0, total_nelem * sizeof(uint32_t));
        #endif

        shmem_sync_all();

        fcollect(dst, src, nelem, 0, 0, npes, pSync);

        #ifdef PRINT
        for (int b = 0; b < npes; b++) {
            shmem_barrier_all();
            if (b != me) {
                continue;
            }

            gprintf("%2d: %2d", shmem_my_pe(), dst[0]);
            for (int j = 1; j < total_nelem; j++) { gprintf(", %2d", dst[j]); }
            gprintf("\n");

            if (me == npes - 1) {
                gprintf("\n");
            }
        }
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

    shmem_free(pSync);
    shmem_free(src);
    shmem_free(dst);

    return (end - start) / 1e9;
}

int main(int argc, char *argv[]) {
    int iterations;
    size_t count;

    time_ns_t start = current_time_ns();
    shmem_init();
    time_ns_t end = current_time_ns();

    int me = shmem_my_pe();
    int npes = shmem_n_pes();

    if (shmem_my_pe() == 0) {
        gprintf("shmem_init: %lf\n", (end - start) / 1e9);
    }

    // @formatter:off

    for (int i = 1; i < argc; i++) {
        sscanf(argv[i], "%d:%zu", &iterations, &count);

        RUN_CSV(fcollect32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_COLLECT_SYNC_SIZE);

        RUNC_CSV(count >= 256, fcollect32, ring, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(npes % 2 == 0 && count >= 32, fcollect32, neighbour_exchange, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(!((npes - 1) & npes), fcollect32, rec_dbl, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);

        RUNC_CSV(count <= 256, fcollect32, bruck, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(count <= 256, fcollect32, bruck_no_rotate, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(count <= 256, fcollect32, bruck_signal, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(0, fcollect32, bruck_inplace, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);

        RUNC_CSV(npes <= 96, fcollect32, linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(count <= 256, fcollect32, all_linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
        RUNC_CSV(count <= 256, fcollect32, all_linear1, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);

        if (shmem_my_pe() == 0) {
            gprintf("\n\n\n\n");
        }
    }

    // @formatter:on

    shmem_finalize();
}