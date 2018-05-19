//
// Created by Srdan Milakovic on 5/17/18.
//

#include "fcollect.h"
#include "barrier.h"
#include "broadcast.h"
#include "../test/util/debug.h"
#include <limits.h>
#include <string.h>
#include <assert.h>

/* TODO: remove void pointer arithmetic */

/**
 *
 * @param dest
 * @param source
 * @param nbytes
 * @param PE_start
 * @param logPE_stride
 * @param PE_size
 * @param pSync pSync should have at least 2 elements
 */
inline static void fcollect_helper_linear(void *dest, const void *source, size_t nbytes, int PE_start,
                                          int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);
    if (me_as != 0) {
        shmem_char_put(dest + me_as * nbytes, source, nbytes, PE_start);
    } else {
        memcpy(dest, source, nbytes);
    }
    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync + 1);

    shcoll_linear_broadcast8(dest, dest, nbytes * shmem_n_pes(), 0, PE_start, logPE_stride, PE_size, pSync + 2);
}

/**
 *
 * @param dest
 * @param source
 * @param nbytes
 * @param PE_start
 * @param logPE_stride
 * @param PE_size
 * @param pSync pSync should have at least ⌈log(max_rank)⌉ elements
 */
inline static void fcollect_helper_rec_dbl(void *dest, const void *source, size_t nbytes, int PE_start,
                                           int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;
    int mask;
    int peer;
    int i;
    int data_block = me_as;

    const long SYNC_VALUE = SHCOLL_SYNC_VALUE;

    assert(((PE_size - 1) & PE_size) == 0);

    memcpy(dest + me_as * nbytes, source, nbytes);

    for (mask = 0x1, i = 0; mask < PE_size; mask <<= 1, i++) {
        peer = PE_start + (me_as ^ mask) * stride;

        shmem_putmem(dest + data_block * nbytes, dest + data_block * nbytes, nbytes * mask, peer);
        shmem_fence();
        shmem_long_atomic_inc(pSync + i, peer);

        data_block &= ~mask;
        //gprintf("%d -> %d [%d, %zu]\n", me, peer, data_block, nbytes * mask);


        shmem_long_wait_until(pSync + i, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
        shmem_long_put(pSync + i, &SYNC_VALUE, 1, me);
    }

    for (mask = 0x1, i = 0; mask < PE_size; mask <<= 1, i++) {
        shmem_long_wait_until(pSync + i, SHMEM_CMP_EQ, SYNC_VALUE);
    }
}

/**
 *
 * @param dest
 * @param source
 * @param nbytes
 * @param PE_start
 * @param logPE_stride
 * @param PE_size
 * @param pSync pSync should have at least 1 element
 */
inline static void fcollect_helper_ring(void *dest, const void *source, size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;
    int peer = (me_as + 1) % PE_size;
    int data_block = me_as;
    int i;

    const long SYNC_VALUE = SHCOLL_SYNC_VALUE;

    memcpy(dest + data_block * nbytes, source, nbytes);

    for (i = 1; i < PE_size; i++) {
        //gprintf("%d -> %d %d %ld\n", me, peer, data_block, *pSync);
        shmem_putmem(dest + data_block * nbytes, dest + data_block * nbytes, nbytes, peer);
        shmem_fence();
        shmem_long_atomic_inc(pSync, peer);

        data_block = (data_block - 1 + PE_size) % PE_size;

        shmem_long_wait_until(pSync, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + i);
    }

    //gprintf("%d %ld\n", me, *pSync);
    shmem_long_put(pSync, &SYNC_VALUE, 1, me);
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SYNC_VALUE);
}


#define SHCOLL_FCOLLECT_DEFINITION(_name, _size)                                                        \
    void shcoll_fcollect##_size##_##_name(void *dest, const void *source, size_t nelems,                \
                                          int PE_start, int logPE_stride, int PE_size, long *pSync) {   \
        fcollect_helper_##_name(dest, source, (_size) / CHAR_BIT * nelems,                              \
                               PE_start, logPE_stride, PE_size, pSync);                                 \
}                                                                                                       \


SHCOLL_FCOLLECT_DEFINITION(linear, 32)
SHCOLL_FCOLLECT_DEFINITION(linear, 64)

SHCOLL_FCOLLECT_DEFINITION(rec_dbl, 32)
SHCOLL_FCOLLECT_DEFINITION(rec_dbl, 64)

SHCOLL_FCOLLECT_DEFINITION(ring, 32)
SHCOLL_FCOLLECT_DEFINITION(ring, 64)