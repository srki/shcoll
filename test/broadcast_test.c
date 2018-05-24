#include "broadcast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util/util.h"
#include "util/debug.h"
#include "util/run.h"


#define VERIFY
#define PRINTx

typedef void (*broadcast_impl)(void *, const void *, size_t, int, int, int, int, long *);

double test_broadcast(broadcast_impl broadcast, int iterations, size_t count, long SYNC_VALUE, size_t BCAST_SYNC_CAST) {
    #ifdef PRINT
    long *order = shmem_calloc(1, sizeof(long));
    #endif

    long *pSync = shmem_malloc(BCAST_SYNC_CAST * sizeof(long));
    for (int i = 0; i < BCAST_SYNC_CAST; i++) {
        pSync[i] = SYNC_VALUE;
    }
    shmem_barrier_all();

    uint32_t *dst = shmem_calloc(count, sizeof(uint32_t));
    uint32_t *src = shmem_calloc(count, sizeof(uint32_t));

    for (int i = 0; i < count; i++) {
        src[i] = (uint32_t) i + 1;
    }

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memset(dst, 0, count * sizeof(uint32_t));
        #endif

        shmem_barrier_all(); /* shmem_sync_all(); */

        int root = i % shmem_n_pes();
        broadcast(dst, src, count, root, 0, 0, shmem_n_pes(), pSync);

        #ifdef PRINT
        shmem_barrier_all();
        shmem_long_wait_until(order, SHMEM_CMP_NE, i);

        gprintf("%d: ", shmem_my_pe());
        for (int j = 0; j < count; ++j) { gprintf("%u ", dst[j]); }
        gprintf("\n");

        if (shmem_my_pe() != root) {
            shmem_long_atomic_inc(order, (shmem_my_pe() + 1) % shmem_n_pes());
        } else {
            shmem_long_atomic_inc(order, shmem_my_pe());
            shmem_long_atomic_inc(order, (shmem_my_pe() + 1) % shmem_n_pes());
        }
        #endif

        #ifdef VERIFY
        if (shmem_my_pe() != root) {
            for (int j = 0; j < count; ++j) {
                if (dst[j] != j + 1) {
                    gprintf("[%d] dst[%d] = %u; expected: %u\n", shmem_my_pe(), j, dst[j], j + 1);
                    abort();
                }
            }
        }
        #endif
    }

    unsigned long long end = current_time_ns();

    shmem_barrier_all();
    shmem_free(pSync);
    shmem_free(dst);
    shmem_free(src);
    shmem_barrier_all();

    return (end - start) / 1e9;
}


int main(int argc, char *argv[]) {
    int iterations = (int) (argc > 1 ? strtol(argv[1], NULL, 0) : 1);
    size_t count = (size_t) (argc > 2 ? strtol(argv[2], NULL, 0) : 1);

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf("PEs: %d\n", shmem_n_pes());
    }

    if (shmem_my_pe() == 0) {

    } else {

    }

    if (shmem_my_pe() == 0) {
        gprintf("shmem: %lf\n", test_broadcast(shmem_broadcast32, iterations, count, SHMEM_SYNC_VALUE, SHMEM_BCAST_SYNC_SIZE));
        gprintf("Linear: %lf\n", test_broadcast(shcoll_linear_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
        gprintf("Binomial: %lf\n", test_broadcast(shcoll_binomial_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
        for (int degree = 2; degree <= 8; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            gprintf("Complete %d: %lf\n", degree, test_broadcast(shcoll_complete_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
        }
        gprintf("Scatter & Collect: %lf\n", test_broadcast(shcoll_scatter_collect_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
    } else {
        test_broadcast(shmem_broadcast32, iterations, count, SHMEM_SYNC_VALUE, SHMEM_BCAST_SYNC_SIZE);
        test_broadcast(shcoll_linear_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        test_broadcast(shcoll_binomial_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        for (int degree = 2; degree <= 8; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            test_broadcast(shcoll_complete_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        }
        test_broadcast(shcoll_scatter_collect_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
    }

    shmem_finalize();
    return 0;
}