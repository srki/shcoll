#include "reduction.h"
#include <stdlib.h>
#include <stdio.h>
#include "util/util.h"
#include "util/debug.h"
#include "util/run.h"

#define VERIFY
#define PRINTx

#define MAX(A, B) ((A) > (B) ? (A) : (B))

typedef void (*reduce_impl)(int *, const int *, int, int, int, int, int *, long *);

static inline void shcoll_int_sum_to_all_shmem(int *dest, const int *source, int nreduce, int PE_start,
                                               int logPE_stride, int PE_size, int *pWrk, long *pSync) {
    shmem_int_sum_to_all(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);
}

double test_int_sum_to_all(reduce_impl reduce, int iterations, size_t count,
                           long SYNC_VALUE, size_t REDUCE_SYNC_SIZE, size_t REDUCE_MIN_WRKDATA_SIZE) {
    long *pSync = shmem_malloc(REDUCE_SYNC_SIZE * sizeof(long));
    int *pWrk = shmem_malloc(MAX(REDUCE_MIN_WRKDATA_SIZE, count) * sizeof(int));

    for (int i = 0; i < REDUCE_SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }

    shmem_barrier_all();

    int npes = shmem_n_pes();
    int me = shmem_my_pe();

    int *dst = shmem_calloc(count, sizeof(int));
    int *src = shmem_calloc(count, sizeof(int));

    for (int i = 0; i < count; i++) {
        src[i] = ((i + 1) % 10007) * (me + 1);
    }

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memset(dst, 0, count * sizeof(uint32_t));
        #endif

        shmem_barrier_all();
        reduce(dst, src, (int) count, 0, 0, npes, pWrk, pSync);

        #ifdef PRINT
        for (int b = 0; b < npes; b++) {
            shmem_barrier_all();
            if (b != me) {
                continue;
            }

            gprintf("%2d: %3d", me, dst[0]);
            for (int j = 1; j < count; j++) { gprintf(", %3d", dst[j]); }
            gprintf("\n");

            if (me == npes - 1) {
                gprintf("\n");
            }
        }
        shmem_barrier_all();
        #endif

        #ifdef VERIFY
        int sum = (npes + 1) * npes / 2;
        for (int j = 0; j < count; j++) {
            if (dst[j] != sum * ((j + 1) % 10007)) {
                gprintf("[%d] i:%d dst[%d] = %d; Expected %d\n", me, i, j, dst[j], sum * ((j + 1) % 10007));
                gprintf("%2d: %3d", me, dst[0]);
                for (int k = 1; k < count; k++) { gprintf(", %3d", dst[k]); }
                gprintf("\n");
                abort();
            }
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
    int iterations = argc > 1 ? (int) strtol(argv[1], NULL, 0) : 1;
    size_t count = argc > 2 ? (size_t) strtol(argv[2], NULL, 0) : 1;

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf("[%s]PEs: %d; size: %zu\n", __FILE__, shmem_n_pes(), count * sizeof(int));
    }

    // @formatter:off

    RUN(int_sum_to_all, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_REDUCE_SYNC_SIZE, SHMEM_REDUCE_MIN_WRKDATA_SIZE);
    RUN(int_sum_to_all, rec_dbl, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE);
    RUN(int_sum_to_all, binomial, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE);
    RUN(int_sum_to_all, rabenseifner, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE);
    RUN(int_sum_to_all, linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_REDUCE_SYNC_SIZE, SHCOLL_REDUCE_MIN_WRKDATA_SIZE);

    // @formatter:on

    shmem_finalize();
}
