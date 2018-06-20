//
// Created by Srdan Milakovic on 5/21/18.
//

#include "alltoalls.h"
#include <string.h>
#include "util/util.h"

#define CSV
#include "util/run.h"

#define VERIFYx
#define PRINTx
#define WARMUP

#define CROP_VALUE 1000
#define CROP(A) ((A) / CROP_VALUE)

typedef void (*alltoalls_impl)(void *, const void *, ptrdiff_t, ptrdiff_t, size_t, int, int, int, long *);

void shcoll_alltoalls32_shmem(void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst, size_t count,
                             int PE_start, int logPE_stride, int PE_size, long *pSync) {
    shmem_alltoalls32(dest, source, dst, sst, count, PE_start, logPE_stride, PE_size, pSync);
}

int verify(const uint32_t *dest, size_t nelems, size_t total_nelems, ptrdiff_t dst) {
    const int me = shmem_my_pe();

    for (int j = 0, e_next = 0, p_next = 0; j < total_nelems; j++) {
        uint32_t value = dest[j * dst];
        int e = value % CROP_VALUE;
        int p = value / (CROP_VALUE * CROP_VALUE);
        int m = (value / CROP_VALUE) % CROP_VALUE;

        if (e != CROP(e_next) || p != CROP(p_next) || m != CROP(me)) {
            gprintf("[%d] dest[%d] = (%d, %d, %d); expected (%d, %d, %d)\n", me, j, e, p, m, e_next, p_next, me);
            return 0;
        }

        if (++e_next == nelems) {
            e_next = 0;
            p_next++;
        }
    }

    return 1;
}

double test_alltoalls32(alltoalls_impl alltoalls, int iterations, size_t nelems, long SYNC_VALUE, size_t SYNC_SIZE) {
    long *pSync = shmem_malloc(SYNC_SIZE * sizeof(long));
    for (int i = 0; i < SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }

    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    size_t total_nelems = npes * nelems;

    const ptrdiff_t dst = 2;
    const ptrdiff_t sst = 3;

    uint32_t *source = shmem_malloc(total_nelems * sst * sizeof(uint32_t));
    uint32_t *dest = shmem_malloc(total_nelems * dst * sizeof(uint32_t));

    // src_PE | dst_PE | idx
    int idx = 0;
    for (int pe = 0; pe < npes; pe++) {
        for (int e = 0; e < nelems; e++) {
            source[idx] = (uint32_t) (CROP(e) + CROP_VALUE * CROP(pe) + CROP_VALUE * CROP_VALUE * CROP(me));
            idx += sst;
        }
    }

    #ifdef WARMUP
    shmem_barrier_all();

    for (int i = 0; i < iterations / 10; i++) {
        memset(dest, 0, total_nelems * dst * sizeof(uint32_t));

        shmem_barrier_all();
        alltoalls(dest, source, dst, sst, nelems, 0, 0, npes, pSync);

        if (!verify(dest, nelems, total_nelems, dst)) {
            return -1.0;
        }
    }
    #endif

    shmem_barrier_all();
    time_ns_t start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #if defined(VERIFY) || defined(PRINT)
        memset(dest, 0, total_nelems * dst * sizeof(uint32_t));
        #endif

        shmem_barrier_all();

        alltoalls(dest, source, dst, sst, nelems, 0, 0, npes, pSync);

        #ifdef PRINT
        for (int b = 0; b < npes; b++) {
            shmem_barrier_all();
            if (b != me) {
                continue;
            }

            gprintf("%d: %03d", me, dest[0]);
            for (int j = 1; j < total_nelems; j++) { gprintf(", %03d", dest[j * dst]); }
            gprintf("\n");
        }
        #endif

        #ifdef VERIFY
        if (!verify(dest, nelems, total_nelems, dst)) {
            return -1.0;
        }
        #endif
    }

    shmem_barrier_all();
    time_ns_t end = current_time_ns();

    shmem_free(pSync);
    shmem_free(source);
    shmem_free(dest);

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

    if (me == 0) {
        gprintf("%s PEs: %d; shmem_init: %lf\n", __FILE__, npes, (end - start) / 1e9);
    }

    // @formatter:off

    for (int i = 1; i < argc; i++) {
        sscanf(argv[i], "%d:%zu", &iterations, &count);

        RUN(alltoalls32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_ALLTOALLS_SYNC_SIZE);
        
        RUNC(0, alltoalls32, shift_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        RUNC(0, alltoalls32, shift_exchange_counter, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        
        RUN(alltoalls32, shift_exchange_barrier_nbi, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        RUNC(npes <= 64, alltoalls32, shift_exchange_counter_nbi, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        
        
        RUNC(0 && ((npes - 1) & npes) == 0, alltoalls32, xor_pairwise_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        RUNC(0 && ((npes - 1) & npes) == 0, alltoalls32, xor_pairwise_exchange_counter, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        
        RUNC(((npes - 1) & npes) == 0, alltoalls32, xor_pairwise_exchange_barrier_nbi, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        RUNC(npes <= 64 && ((npes - 1) & npes) == 0, alltoalls32, xor_pairwise_exchange_counter_nbi, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        
        
        RUNC(0, alltoalls32, color_pairwise_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        RUNC(0, alltoalls32, color_pairwise_exchange_counter, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        
        RUN(alltoalls32, color_pairwise_exchange_barrier_nbi, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);
        RUNC(npes <= 64, alltoalls32, color_pairwise_exchange_counter_nbi, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALLS_SYNC_SIZE);

        if (me == 0) {
            gprintf("\n\n\n\n");
        }
    }

    // @formatter:on

    shmem_finalize();

    return 0;
}