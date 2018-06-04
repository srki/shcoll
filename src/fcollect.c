//
// Created by Srdan Milakovic on 5/17/18.
//

#include "fcollect.h"
#include "barrier.h"
#include "broadcast.h"
#include "../test/util/debug.h"
#include "util/rotate.h"
#include <limits.h>
#include <string.h>
#include <assert.h>

/* TODO: remove void pointer arithmetic */
/* TODO: fix possible overflows with int and size_t */

/**
 @param pSync pSync should have at least 2 elements
 */
inline static void fcollect_helper_linear(void *dest, const void *source, size_t nbytes, int PE_start,
                                          int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);
    if (me_as != 0) {
        shmem_char_put(dest + me_as * nbytes, source, nbytes, PE_start);
    } else {
        memcpy(dest, source, nbytes);
    }
    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);

    shcoll_broadcast8_linear(dest, dest, nbytes * shmem_n_pes(), 0, PE_start, logPE_stride, PE_size, pSync + 1);
}

/**
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
        shmem_long_atomic_inc(pSync + i, peer); /* TODO: try to use put */

        data_block &= ~mask;

        shmem_long_wait_until(pSync + i, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
        shmem_long_put(pSync + i, &SYNC_VALUE, 1, me);
    }

    for (mask = 0x1, i = 0; mask < PE_size; mask <<= 1, i++) {
        shmem_long_wait_until(pSync + i, SHMEM_CMP_EQ, SYNC_VALUE);
    }
}

/**
 * @param pSync pSync should have at least 1 element
 */
inline static void fcollect_helper_ring(void *dest, const void *source, size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;
    int peer = PE_start + ((me_as + 1) % PE_size) * stride;
    int data_block = me_as;
    int i;

    const long SYNC_VALUE = SHCOLL_SYNC_VALUE;

    memcpy(dest + data_block * nbytes, source, nbytes);

    for (i = 1; i < PE_size; i++) {
        shmem_putmem(dest + data_block * nbytes, dest + data_block * nbytes, nbytes, peer);
        shmem_fence();
        shmem_long_atomic_inc(pSync, peer); /* TODO: try to use put */

        data_block = (data_block - 1 + PE_size) % PE_size;
        shmem_long_wait_until(pSync, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + i);
    }

    shmem_long_put(pSync, &SYNC_VALUE, 1, me);
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SYNC_VALUE);
}

/**
 * @param pSync pSync should have at least ⌈log(max_rank)⌉ elements
 */
inline static void fcollect_helper_bruck(void *dest, const void *source, size_t nbytes, int PE_start,
                                         int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;
    size_t distance;
    int round;
    int peer;
    size_t sent_bytes = nbytes;
    size_t total_nbytes = PE_size * nbytes;
    size_t to_send;

    const long SYNC_VALUE = SHCOLL_SYNC_VALUE;

    memcpy(dest, source, nbytes);

    for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
        peer = (int) (PE_start + ((me_as - distance + PE_size) % PE_size) * stride);
        to_send = (2 * sent_bytes <= total_nbytes) ? sent_bytes : total_nbytes - sent_bytes;

        shmem_putmem(dest + sent_bytes, dest, to_send, peer);
        shmem_fence();
        shmem_long_atomic_inc(pSync + round, peer); /* TODO: try to use put */

        sent_bytes += distance * nbytes;
        shmem_long_wait_until(pSync + round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
        shmem_long_put(pSync + round, &SYNC_VALUE, 1, me);
    }

    rotate(dest, total_nbytes, me_as * nbytes);

    for (distance = 1, round = 0; distance < PE_size; distance <<= 2, round++) {
        shmem_long_wait_until(pSync + round, SHMEM_CMP_EQ, SYNC_VALUE);
    }
}

/**
 * @param pSync pSync should have at least 2 elements
 */
inline static void fcollect_helper_neighbour_exchange(void *dest, const void *source, size_t nbytes, int PE_start,
                                                      int logPE_stride, int PE_size, long *pSync) {
    assert(PE_size % 2 == 0);

    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    int neighbour_pe[2];
    int send_offset[2];
    int send_offset_diff;

    int i, parity;
    void *data;

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    if (me_as % 2 == 0) {
        neighbour_pe[0] = PE_start + ((me_as + 1) % PE_size) * stride;
        neighbour_pe[1] = PE_start + ((me_as - 1 + PE_size) % PE_size) * stride;

        send_offset[0] = (me_as - 2 + PE_size) % PE_size & ~0x1;
        send_offset[1] = me_as & ~0x1;

        send_offset_diff = 2;
    } else {
        neighbour_pe[0] = PE_start + ((me_as - 1 + PE_size) % PE_size) * stride;
        neighbour_pe[1] = PE_start + ((me_as + 1) % PE_size) * stride;

        send_offset[0] = (me_as + 2) % PE_size & ~0x1;
        send_offset[1] = me_as & ~0x1;

        send_offset_diff = -2 + PE_size;
    }

    /* First round */
    data = ((char *) dest) + me_as * nbytes;

    memcpy(data, source, nbytes);

    shmem_putmem(data, data, nbytes, neighbour_pe[0]);
    shmem_fence();
    shmem_long_atomic_inc(pSync, neighbour_pe[0]);

    shmem_long_wait_until(pSync, SHMEM_CMP_GE, 1);

    /* Remaining npes/2 - 1 rounds */
    for (i = 1; i < PE_size / 2; i++) {
        parity = i % 2 ? 1 : 0;
        data = ((char *) dest) + send_offset[parity] * nbytes;

        /* Send data */
        shmem_putmem(data, data, 2 * nbytes, neighbour_pe[parity]);
        shmem_fence();
        shmem_long_atomic_inc(pSync + parity, neighbour_pe[parity]);

        /* Calculate offset for the next round */
        send_offset[parity] = (send_offset[parity] + send_offset_diff) % PE_size;
        send_offset_diff = PE_size - send_offset_diff;

        /* Wait for the data from the neighbour */
        shmem_long_wait_until(pSync + parity, SHMEM_CMP_GT, i / 2);
    }

    pSync[0] = SHCOLL_SYNC_VALUE;
    pSync[1] = SHCOLL_SYNC_VALUE;
}

#define SHCOLL_FCOLLECT_DEFINITION(_name, _size)                                                        \
    void shcoll_fcollect##_size##_##_name(void *dest, const void *source, size_t nelems,                \
                                          int PE_start, int logPE_stride, int PE_size, long *pSync) {   \
        fcollect_helper_##_name(dest, source, (_size) / CHAR_BIT * nelems,                              \
                               PE_start, logPE_stride, PE_size, pSync);                                 \
}                                                                                                       \

/* @formatter:off */

SHCOLL_FCOLLECT_DEFINITION(linear, 32)
SHCOLL_FCOLLECT_DEFINITION(linear, 64)

SHCOLL_FCOLLECT_DEFINITION(rec_dbl, 32)
SHCOLL_FCOLLECT_DEFINITION(rec_dbl, 64)

SHCOLL_FCOLLECT_DEFINITION(ring, 32)
SHCOLL_FCOLLECT_DEFINITION(ring, 64)

SHCOLL_FCOLLECT_DEFINITION(bruck, 32)
SHCOLL_FCOLLECT_DEFINITION(bruck, 64)

SHCOLL_FCOLLECT_DEFINITION(neighbour_exchange, 32)
SHCOLL_FCOLLECT_DEFINITION(neighbour_exchange, 64)

/* @formatter:on */
