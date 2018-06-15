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

typedef void (*alltoall_impl)(void *, const void *, size_t, int, int, int, long *);

void shcoll_alltoall32_shmem(void *dest, const void *source, size_t count,
                             int PE_start, int logPE_stride, int PE_size, long *pSync) {
    shmem_alltoall32(dest, source, count, PE_start, logPE_stride, PE_size, pSync);
}

double test_alltoall32(alltoall_impl alltoall, int iterations, size_t nelems, long SYNC_VALUE, size_t SYNC_SIZE) {
    #ifdef PRINT
    long *lock = shmem_malloc(sizeof(long));
    *lock = 0;
    #endif

    long *pSync = shmem_malloc(SYNC_SIZE * sizeof(long));
    for (int i = 0; i < SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }

    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    size_t total_nelem = npes * nelems;

    uint32_t *src = shmem_malloc(total_nelem * sizeof(uint32_t));
    uint32_t *dst = shmem_malloc(total_nelem * sizeof(uint32_t));

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

    shmem_barrier_all();
    time_ns_t start = current_time_ns();


    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memset(dst, 0, total_nelem * sizeof(uint32_t));
        #endif

        shmem_sync_all();

        alltoall(dst, src, nelems, 0, 0, npes, pSync);

        #ifdef PRINT
        shmem_set_lock(lock);
        printf("%d: %03d", me, dst[0]);
        for (int j = 1; j < total_nelem; j++) { printf(", %03d", dst[j]); }
        printf("\n");
        shmem_clear_lock(lock);
        #endif

        #ifdef VERIFY
        for (int j = 0, e_next = 0, p_next = 0; j < total_nelem; j++) {
            uint32_t value = dst[j];
            int e = value % base_nelems;
            int p = value / (base_nelems * base_npes);
            int m = (value / base_nelems) % base_npes;


            if (e != e_next || p != p_next || m != me) {
                gprintf("[%d] dst[%d] = (%d, %d, %d); expected (%d, %d, %d)\n", me, j, e, p, m, e_next, p_next, me);
                abort();
            }

            if (++e_next == nelems) {
                e_next = 0;
                p_next++;
            }

        }
        #endif
    }

    shmem_barrier_all();
    time_ns_t end = current_time_ns();

    #ifdef PRINT
    shmem_free(lock);
    #endif

    return (end - start) / 1e9;
}

int main(int argc, char *argv[]) {
    int iterations = (int) (argc > 1 ? strtol(argv[1], NULL, 0) : 1);
    size_t count = (size_t) (argc > 2 ? strtol(argv[2], NULL, 0) : 1);

    shmem_init();

    if (shmem_my_pe() == 0) {
        gprintf(__FILE__ " PEs: %d\n", shmem_n_pes());
    }

    RUN(alltoall32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_ALLTOALL_SYNC_SIZE);
    RUN(alltoall32, shift_exchange_barrier, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_ALLTOALL_SYNC_SIZE);


    shmem_finalize();

    return 0;
}