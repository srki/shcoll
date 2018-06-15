//
// Created by Srdan Milakovic on 5/21/18.
//

#include "alltoall.h"
#include <stdlib.h>
#include <string.h>
#include "util/run.h"
#include "util/util.h"

#define VERIFY
#define PRINTx
#define WARMUP

typedef void (*alltoall_impl)(void *, const void *, size_t, int, int, int, long *);

void shcoll_alltoall32_shmem(void *dest, const void *source, size_t count,
                             int PE_start, int logPE_stride, int PE_size, long *pSync) {
    shmem_alltoall32(dest, source, count, PE_start, logPE_stride, PE_size, pSync);
}

int verify(const uint32_t *dst, size_t nelems, size_t total_nelems, int base_npes, int base_nelems) {
    int me = shmem_my_pe();

    for (int j = 0, e_next = 0, p_next = 0; j < total_nelems; j++) {
        uint32_t value = dst[j];
        int e = value % base_nelems;
        int p = value / (base_nelems * base_npes);
        int m = (value / base_nelems) % base_npes;


        if (e != e_next || p != p_next || m != me) {
            gprintf("[%d] dst[%d] = (%d, %d, %d); expected (%d, %d, %d)\n", me, j, e, p, m, e_next, p_next, me);
            return 0;
        }

        if (++e_next == nelems) {
            e_next = 0;
            p_next++;
        }
    }

    return 1;
}

double test_alltoall32(alltoall_impl alltoall, int iterations, size_t nelems, long SYNC_VALUE, size_t SYNC_SIZE) {
    long *pSync = shmem_malloc(SYNC_SIZE * sizeof(long));
    for (int i = 0; i < SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }

    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    size_t total_nelems = npes * nelems;

    uint32_t *src = shmem_malloc(total_nelems * sizeof(uint32_t));
    uint32_t *dst = shmem_malloc(total_nelems * sizeof(uint32_t));

    int base_npes = 10;
    while (base_npes <= npes) base_npes *= 10;

    int base_nelems = 10;
    while (base_nelems <= nelems) base_nelems *= 10;

    // src_PE | dst_PE | idx
    int idx = 0;
    for (int pe = 0; pe < npes; pe++) {
        for (int e = 0; e < nelems; e++) {
            src[idx++] = (uint32_t) (e + base_nelems * pe + base_nelems * base_npes * me);
        }
    }

    #ifdef WARMUP
    shmem_barrier_all();

    for (int i = 0; i < iterations / 10; i++) {
        memset(dst, 0, total_nelems * sizeof(uint32_t));

        shmem_barrier_all();
        alltoall(dst, src, nelems, 0, 0, npes, pSync);

        if (!verify(dst, nelems, total_nelems, base_npes, base_nelems)) {
            abort();
        }
    }
    #endif

    shmem_barrier_all();
    time_ns_t start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #if defined(VERIFY) | defined(PRINT)
        memset(dst, 0, total_nelems * sizeof(uint32_t));
        #endif

        shmem_barrier_all();

        alltoall(dst, src, nelems, 0, 0, npes, pSync);

        #ifdef PRINT
        for (int b = 0; b < npes; b++) {
            shmem_barrier_all();
            if (b != me) {
                continue;
            }

            gprintf("%d: %03d", me, dst[0]);
            for (int j = 1; j < total_nelems; j++) { gprintf(", %03d", dst[j]); }
            gprintf("\n");
        }
        #endif

        #ifdef VERIFY
        if (!verify(dst, nelems, total_nelems, base_npes, base_nelems)) {
            abort();
        }
        #endif
    }

    shmem_barrier_all();
    time_ns_t end = current_time_ns();

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

        RUN(alltoall32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_ALLTOALL_SYNC_SIZE);

        RUN(alltoall32, shift_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);
        RUN(alltoall32, shift_exchange_counter, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);
        RUNC(npes + 1 <= SHCOLL_ALLTOALL_SYNC_SIZE, alltoall32, shift_exchange_signal, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);

        RUNC(((npes - 1) & npes) == 0, alltoall32, xor_pairwise_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);
        RUNC(((npes - 1) & npes) == 0, alltoall32, xor_pairwise_exchange_counter, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);
        RUNC(((npes - 1) & npes) == 0 && npes + 1 <= SHCOLL_ALLTOALL_SYNC_SIZE, alltoall32, xor_pairwise_exchange_signal, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);

        RUN(alltoall32, color_pairwise_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);
        RUN(alltoall32, color_pairwise_exchange_counter, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);
        RUN(alltoall32, color_pairwise_exchange_signal, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);

        RUN(alltoall32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_ALLTOALL_SYNC_SIZE);

        if (me == 0) {
            gprintf("\n\n\n\n");
        }
    }

    // @formatter:on

    shmem_finalize();

    return 0;
}