#include "reduction.h"
#include <shmem.h>
#include <stdlib.h>
#include <string.h>
#include "broadcast.h"

/*
 * Linear reduction implementation
 */

typedef int T;

inline static void int_sum_helper_linear(T *dest, const T *source, int nreduce, int PE_start,
                                         int logPE_stride, int PE_size, T *pWrk, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();
    int me_as = (me - PE_start) / stride;

    size_t nbytes = sizeof(T) * nreduce;
    T *tmp_dest;

    int i, j;

    shmem_barrier(PE_start, logPE_stride, PE_size, pSync); /* TODO: use shcoll barrier? */

    if (me_as == 0) {
        tmp_dest = malloc(nbytes);
        if (!tmp_dest) {
            /* TODO: raise error */
            exit(0);
        }

        memcpy(tmp_dest, source, nbytes);

        for (i = 1; i < PE_size; i++) {
            shmem_getmem(dest, source, nbytes, PE_start + i * stride);
            for (j = 0; j < nreduce; j++) {
                tmp_dest[j] += dest[j];
            }
        }

        memcpy(dest, tmp_dest, nbytes);
        free(tmp_dest);
    }

    shmem_barrier(PE_start, logPE_stride, PE_size, pSync); /* TODO: use shcoll barrier? */
}

void int_sum_to_all_helper_linear(T *dest, const T *source, int nreduce, int PE_start,
                                  int logPE_stride, int PE_size, T *pWrk, long *pSync) {
    int_sum_helper_linear(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);

    shcoll_binomial_tree_broadcast8(dest, dest, nreduce * sizeof(T),
                                    PE_start, PE_start, logPE_stride, PE_size, pSync);
}