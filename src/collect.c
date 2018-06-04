//
// Created by Srdan Milakovic on 5/15/18.
//

#include "collect.h"
#include <string.h>
#include <limits.h>
#include "util/rotate.h"
#include "util/scan.h"
#include "barrier.h"
#include "broadcast.h"
#include "../test/util/run.h"

inline static void collect_helper_linear(void *dest, const void *source, size_t nbytes, int PE_start,
                                         int logPE_stride, int PE_size, long *pSync) {
    /* pSync[0] is used for barrier
     * pSync[1] is used for broadcast
     * next sizeof(size_t) bytes are used for the offset */

    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();
    const int me_as = (me - PE_start) / stride;
    size_t *offset = (size_t *) (pSync + 2);
    int i;

    /* set offset to 0 */
    offset[0] = 0;
    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);

    if (me_as == 0) {
        shmem_size_atomic_add(offset, nbytes + 1, me + stride);
        memcpy(dest, source, nbytes);

        /* Wait for the full array size and notify everybody */
        shmem_size_wait_until(offset, SHMEM_CMP_NE, 0);

        /* Send offset to everybody */
        for (i = 1; i < PE_size; i++) {
            shmem_size_put(offset, offset, 1, PE_start + i * stride);
        }
    } else {
        shmem_size_wait_until(offset, SHMEM_CMP_NE, 0);

        /* Send offset to the next PE, PE_start will contain full array size */
        shmem_size_atomic_add(offset, nbytes + *offset, PE_start + ((me_as + 1) % PE_size) * stride);

        /* Write data to PE 0 */
        shmem_char_put((char *) dest + *offset - 1, source, nbytes, PE_start);
    }

    /* Wait for all PEs to send the data to PE_start */
    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);

    // TODO: fix
    shcoll_broadcast64_linear(offset, offset, 1, PE_start, PE_start, logPE_stride, PE_size, pSync + 4);

    shcoll_broadcast8_linear(dest, dest, *offset - 1, PE_start, PE_start, logPE_stride, PE_size, pSync + 1);
}

/* TODO Find a better way to choose this value */
#define RING_DIFF 10

inline static void collect_helper_ring(void *dest, const void *source, size_t nbytes, int PE_start,
                                       int logPE_stride, int PE_size, long *pSync) {
    /*
     * pSync[0] is used for barrier
     * pSync[1..] is used for exclusive prefix sum
     * pSync[1] is to track the progress of the left PE
     * pSync[2..] is used to receive block sizes
     */
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    int me_as = (me - PE_start) / stride;
    int recv_from_pe = PE_start + ((me_as + 1) % PE_size) * stride;
    int send_to_pe = PE_start + ((me_as - 1 + PE_size) % PE_size) * stride;

    int round;
    long *receiver_progress = pSync + 1;
    size_t *block_sizes = (size_t *) (pSync + 2);
    size_t *block_size_round;
    size_t nbytes_round = nbytes;

    size_t block_offset;

    exclusive_prefix_sum(&block_offset, nbytes, PE_start, logPE_stride, PE_size, pSync + 1);
    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);

    memcpy(((char*) dest) + block_offset, source, nbytes_round);

    for (round = 0; round < PE_size - 1; round++) {

        shmem_putmem_nbi(((char *) dest) + block_offset, ((char *) dest) + block_offset, nbytes_round , send_to_pe);
        shmem_fence();

        /* Wait until it's safe to use block_size buffer */
        shmem_long_wait_until(receiver_progress, SHMEM_CMP_GT, round - RING_DIFF + SHCOLL_SYNC_VALUE);
        block_size_round = block_sizes + (round % RING_DIFF);

        // TODO: fix -> shmem_size_p(block_size_round, nbytes_round + 1 + SHCOLL_SYNC_VALUE, send_to_pe);
        // shmem_size_atomic_add(block_size_round, nbytes_round + 1 + SHCOLL_SYNC_VALUE, send_to_pe);
        shmem_size_atomic_set(block_size_round, nbytes_round + 1 + SHCOLL_SYNC_VALUE, send_to_pe);

        /* If writing block 0, reset offset to 0 */
        block_offset = (me_as + round + 1 == PE_size) ? 0 : block_offset + nbytes_round;

        /* Wait to receive the data in this round */
        shmem_size_wait_until(block_size_round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
        nbytes_round = *block_size_round - 1 - SHCOLL_SYNC_VALUE;

        /* Reset the block size from the current round */
        shmem_size_p(block_size_round, SHCOLL_SYNC_VALUE, me);
        shmem_size_wait_until(block_size_round, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);

        /* Notify sender that one counter is freed */
        shmem_long_atomic_inc(receiver_progress, recv_from_pe);
    }

    /* Must be atomic fadd because there may be some PE that did not finish with sends */
    shmem_long_atomic_add(receiver_progress, -round, me);
}

inline static void collect_helper_bruck(void *dest, const void *source, size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;
    size_t distance;
    int round;
    int send_to;
    int recv_from;
    size_t recv_nbytes = nbytes;
    size_t round_nbytes;

    size_t *block_sizes = (size_t *) (pSync + 3);

    size_t block_offset;
    size_t total_nbytes;

    // Calculate prefix sum
    exclusive_prefix_sum(&block_offset, nbytes, PE_start, logPE_stride, PE_size, pSync + 1);
    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);

    // Broadcast the total size
    shmem_long_p(pSync + 1, block_offset + nbytes, me);
    shmem_long_wait_until(pSync + 1, SHMEM_CMP_EQ, block_offset + nbytes);

    shcoll_broadcast64_binomial_tree(pSync + 1, pSync + 1, 1, PE_start + stride * (PE_size - 1),
                                     PE_start, logPE_stride, PE_size, pSync + 2);

    total_nbytes = (size_t) *(pSync + 1);

    /* Copy the local block to the destination */
    memcpy(dest, source, nbytes);

    for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
        send_to = (int) (PE_start + ((me_as - distance + PE_size) % PE_size) * stride);
        recv_from = (int) (PE_start + ((me_as + distance) % PE_size) * stride);

        /* Notify partner that the data is ready */
        shmem_size_atomic_set(block_sizes + round, recv_nbytes + 1 + SHCOLL_SYNC_VALUE, send_to);

        /* Wait until the data is ready to be read */
        shmem_size_wait_until(block_sizes + round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
        round_nbytes = *(block_sizes + round) - 1 - SHCOLL_SYNC_VALUE;

        round_nbytes = recv_nbytes + round_nbytes < total_nbytes ? round_nbytes : total_nbytes - recv_nbytes;
        shmem_getmem(dest + recv_nbytes, dest, round_nbytes, recv_from);
        recv_nbytes += round_nbytes;

        /* Reset the block size from the current round */
        shmem_size_p(block_sizes + round, SHCOLL_SYNC_VALUE, me);
        shmem_size_wait_until(block_sizes + round, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
    }

    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);

    *(pSync + 1) = SHCOLL_SYNC_VALUE;

    rotate(dest, total_nbytes, block_offset);
}

#define SHCOLL_COLLECT_DEFINITION(_name, _size)                                                         \
    void shcoll_collect##_size##_##_name(void *dest, const void *source, size_t nelems,                 \
                                         int PE_start, int logPE_stride, int PE_size, long *pSync) {    \
        collect_helper_##_name(dest, source, (_size) / CHAR_BIT * nelems,                               \
                               PE_start, logPE_stride, PE_size, pSync);                                 \
}                                                                                                       \


/* @formatter:off */

SHCOLL_COLLECT_DEFINITION(linear, 32)
SHCOLL_COLLECT_DEFINITION(linear, 64)

SHCOLL_COLLECT_DEFINITION(ring, 32)
SHCOLL_COLLECT_DEFINITION(ring, 64)

SHCOLL_COLLECT_DEFINITION(bruck, 32)
SHCOLL_COLLECT_DEFINITION(bruck, 64)

/* @formatter:on */
