#include "broadcast.h"

#include <time.h>
#include <stdio.h>
#include <shmem.h>
#include <string.h>

#define VERIFY

unsigned long long current_time_ns() {
    struct timespec t = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    unsigned long long s = 1000000000ULL * (unsigned long long) t.tv_sec;
    return (((unsigned long long) t.tv_nsec)) + s;
}

#define gprintf(...) fprintf(stderr, __VA_ARGS__)

typedef void (*broadcast_impl)(void *, const void *, size_t, int, int, int, int, long *);

double test_broadcast(broadcast_impl broadcast, int iterations, size_t count) {
    long *pSync = shmem_malloc(SHCOLL_BCAST_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < SHCOLL_BCAST_SYNC_SIZE; i++) {
        pSync[i] = SHCOLL_SYNC_VALUE;
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

        shmem_sync_all();
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

    if (shmem_my_pe()) {
        gprintf("shmem: %lf\n", test_broadcast(shmem_broadcast32, iterations, count));
        gprintf("Linear: %lf\n", test_broadcast(shcoll_linear_broadcast32, iterations, count));
        gprintf("Binomial: %lf\n", test_broadcast(shcoll_binomial_tree_broadcast32, iterations, count));
        for (int degree = 2; degree <= 8; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            gprintf("Complete %d: %lf\n", degree, test_broadcast(shcoll_complete_tree_broadcast32, iterations, count));
        }
    } else {
        test_broadcast(shmem_broadcast32, iterations, count);
        test_broadcast(shcoll_linear_broadcast32, iterations, count);
        test_broadcast(shcoll_binomial_tree_broadcast32, iterations, count);
        for (int degree = 2; degree <= 8; degree *= 2) {
            shcoll_set_broadcast_tree_degree(degree);
            test_broadcast(shcoll_complete_tree_broadcast32, iterations, count);
        }
    }

    shmem_finalize();
    return 0;
}