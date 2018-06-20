#include "broadcast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util/util.h"
#include "util/debug.h"
#include "util/run.h"

#define VERIFYx
#define PRINTx
#define WARMUP

typedef void (*broadcast_impl)(void *, const void *, size_t, int, int, int, int, long *);

static inline void shcoll_broadcast32_shmem(void *dest, const void *source, size_t nelems, int PE_root,
                              int PE_start, int logPE_stride, int PE_size, long *pSync) {
    shmem_broadcast32(dest, source, nelems, PE_root, PE_start, logPE_stride, PE_size, pSync);
}

int verify(const uint32_t *dest, size_t nelem, int root) {
    const int me = shmem_my_pe();

    if (me != root) {
        for (int j = 0; j < nelem; ++j) {
            if (dest[j] != j + 1) {
                gprintf("[%d] dst[%d] = %u; expected: %u\n", me, j, dest[j], j + 1);
                abort();
            }
        }
    } else {
        for (int j = 0; j < nelem; ++j) {
            if (dest[j] != 0) {
                gprintf("[%d] dst[%d] = %u; expected: %u\n", me, j, dest[j], 0);
                abort();
            }
        }
    }

    return 1;
}

double test_broadcast32(broadcast_impl broadcast, int iterations, size_t count, long SYNC_VALUE, size_t BCAST_SYNC_CAST) {
    long *pSync = shmem_malloc(BCAST_SYNC_CAST * sizeof(long));
    for (int i = 0; i < BCAST_SYNC_CAST; i++) {
        pSync[i] = SYNC_VALUE;
    }
    shmem_barrier_all();

    int npes = shmem_n_pes();
    int me = shmem_my_pe();

    uint32_t *dest = shmem_calloc(count, sizeof(uint32_t));
    uint32_t *source = shmem_calloc(count, sizeof(uint32_t));

    for (int i = 0; i < count; i++) {
        source[i] = (uint32_t) i + 1;
    }

    #ifdef WARMUP
    shmem_barrier_all();

    for (int i = 0; i < iterations / 10; i++) {
        memset(dest, 0, count * sizeof(uint32_t));

        int root = i % npes;
        shmem_barrier_all();
        broadcast(dest, source, count, root, 0, 0, npes, pSync);

        if (!verify(dest, count, root)) {
            return -1.0;
        }
    }
    #endif

    shmem_barrier_all();
    time_ns_t start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #if defined(VERIFY) || defined(PRINT)
        memset(dest, 0, count * sizeof(uint32_t));
        #endif

        shmem_barrier_all();

        int root = i % npes;
        broadcast(dest, source, count, root, 0, 0, npes, pSync);
        #ifdef PRINT
        shmem_barrier_all();

        for (int b = 0; b < npes; b++) {
            shmem_barrier_all();
            if (b != me) {
                continue;
            }

            gprintf("%d: ", me);
            for (int j = 0; j < count; ++j) { gprintf("%u ", dest[j]); }
            gprintf("\n");
        }
        #endif

        #ifdef VERIFY
        if (!verify(dest, count, root)) {
            return -1.0;
        }
        #endif
    }

    time_ns_t end = current_time_ns();

    shmem_barrier_all();
    shmem_free(pSync);
    shmem_free(dest);
    shmem_free(source);
    shmem_barrier_all();

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

        RUN(broadcast32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_BCAST_SYNC_SIZE);
        RUNC(npes <= 2 * 24, broadcast32, linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        RUNC(count > 16384, broadcast32, scatter_collect, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        RUN(broadcast32, binomial_tree, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);

        for (int radix = 2; radix <= 32; radix *= 2) {
            shcoll_set_broadcast_knomial_tree_radix_barrier(radix);
            if (me == 0) gprintf("%2d-", radix);
            RUNC(count <= 262144, broadcast32, knomial_tree, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        }

        for (int radix = 2; radix <= 32; radix *= 2) {
            shcoll_set_broadcast_knomial_tree_radix_barrier(radix);
            if (me == 0) gprintf("%2d-", radix);
            RUNC(count <= 262144, broadcast32, knomial_tree_signal, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        }

        for (int degree = 2; degree <= 32; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            if (me == 0) gprintf("%2d-", degree);
            RUNC(count <= 262144, broadcast32, complete_tree, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        }
    }

    // @formatter:on

    shmem_finalize();
}