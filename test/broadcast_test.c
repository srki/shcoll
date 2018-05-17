#include "broadcast.h"
#include <stdio.h>
#include <string.h>
#include "util/util.h"
#include "util/debug.h"


#define VERIFY

typedef void (*broadcast_impl)(void *, const void *, size_t, int, int, int, int, long *);

double test_broadcast(broadcast_impl broadcast, int iterations, size_t count, long SYNC_VALUE, size_t BCAST_SYNC_CAST) {
    long *pSync = shmem_malloc(BCAST_SYNC_CAST * sizeof(long));
    for (int i = 0; i < BCAST_SYNC_CAST; i++) {
        pSync[i] = SYNC_VALUE;
    }
    shmem_barrier_all();

    uint32_t *dst = shmem_calloc(count, sizeof(uint32_t));
    uint32_t *src = shmem_calloc(count, sizeof(uint32_t));

    for (int i = 0; i < count; i++) {
        src[i] = (uint32_t) i;
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

        #ifdef VERIFY
        if (shmem_my_pe() != root && memcmp(dst, src, count * sizeof(uint32_t)) != 0) {
            printf("%d: ", shmem_my_pe());
            for (int j = 0; j < count; ++j) { printf("%u,%u ", dst[j], src[j]); }
            printf("\n");
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
    int iterations = 100;
    size_t count = 8;

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf("PEs: %d\n", shmem_n_pes());
    }

    if (shmem_my_pe() == 0) {
        gprintf("shmem: %lf\n", test_broadcast(shmem_broadcast32, iterations, count, SHMEM_SYNC_VALUE, SHMEM_BCAST_SYNC_SIZE));
        gprintf("Linear: %lf\n", test_broadcast(shcoll_linear_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
        gprintf("Binomial: %lf\n", test_broadcast(shcoll_binomial_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
        for (int degree = 2; degree <= 8; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            gprintf("Complete %d: %lf\n", degree, test_broadcast(shcoll_complete_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE));
        }
    } else {
        test_broadcast(shmem_broadcast32, iterations, count, SHMEM_SYNC_VALUE, SHMEM_BCAST_SYNC_SIZE);
        test_broadcast(shcoll_linear_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        test_broadcast(shcoll_binomial_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        for (int degree = 2; degree <= 8; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            test_broadcast(shcoll_complete_tree_broadcast32, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_BCAST_SYNC_SIZE);
        }
    }

    shmem_finalize();
    return 0;
}