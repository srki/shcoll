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

#include <stdio.h>

void int_sum_to_all_helper_linear(T *dest, const T *source, int nreduce, int PE_start,
                                  int logPE_stride, int PE_size, T *pWrk, long *pSync) {
    int_sum_helper_linear(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);
    shcoll_linear_broadcast8(dest, dest, nreduce * sizeof(T),
                             PE_start, PE_start, logPE_stride, PE_size, pSync + 1);
}

inline static void int_sum_helper_rec_dbl(T *dest, const T *source, int nreduce, int PE_start,
                                          int logPE_stride, int PE_size, T *pWrk, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();
    int me_as = (me - PE_start) / stride;
    int target_as;
    size_t nbytes = sizeof(T) * nreduce;
    T *tmp_dest;
    int i;
    unsigned mask = 0x1;
    long old_pSync = SHMEM_SYNC_VALUE;
    long to_receive = 0;
    long recv_mask;

    tmp_dest = malloc(nbytes);
    if (!tmp_dest) {
        /* TODO: raise error */
        exit(0);
    }

    if (source != dest) {
        memcpy(dest, source, nbytes);
    }

    /* Stop if all messages are received or if there are no more PE on right */
    for (mask = 0x1; !(me_as & mask) && ((me_as | mask) < PE_size); mask <<= 1) {
        to_receive |= mask;
    }


    // TODO: fix if SHMEM_SYNC_VALUE not 0
    /* Wait until all messages are received */
    while (to_receive != 0) {
        memcpy(tmp_dest, dest, nbytes);
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, old_pSync);
        recv_mask = shmem_long_atomic_fetch(pSync, me);

        recv_mask &= to_receive;
        recv_mask ^= (recv_mask - 1) & recv_mask;

        /* Get array and reduce */
        target_as = (int) (me_as | recv_mask);
        shmem_getmem(dest, dest, nbytes, PE_start + target_as * stride);

        for (i = 0; i < nreduce; i++) {
            dest[i] += tmp_dest[i];
        }

        /* Mark as received */
        to_receive &= ~recv_mask;
        old_pSync |= recv_mask;
    }

    /* Notify parent */
    if (me_as != 0) {
        target_as = me_as & (me_as - 1);
        shmem_long_atomic_add(pSync, me_as ^ target_as, PE_start + target_as * stride);
    }

    //while (1);
    *pSync = SHMEM_SYNC_VALUE;
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync + 1); /* TODO: use shcoll barrier? */
}

void int_sum_to_all_helper_rec_dbl(int *dest, const int *source, int nreduce, int PE_start,
                                   int logPE_stride, int PE_size, int *pWrk, long *pSync) {
    int_sum_helper_rec_dbl(dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);

    shcoll_binomial_tree_broadcast8(dest, dest, nreduce * sizeof(T),
                                    PE_start, PE_start, logPE_stride, PE_size, pSync + 2);
}
