//
// Created by Srdan Milakovic on 5/15/18.
//

#include "collect.h"
#include <string.h>
#include <limits.h>
#include "util/scan.h"
#include "barrier.h"
#include "broadcast.h"

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
    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);

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
    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);

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

    const long SYNC_VALUE = SHCOLL_SYNC_VALUE;

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
    shcoll_binomial_tree_barrier(PE_start, logPE_stride, PE_size, pSync);

    memcpy(((char*) dest) + block_offset, source, nbytes_round);

    for (round = 0; round < PE_size - 1; round++) {

        shmem_putmem_nbi(((char *) dest) + block_offset, ((char *) dest) + block_offset, nbytes_round , send_to_pe);
        shmem_fence();

        /* Wait until it's safe to use block_size buffer */
        shmem_long_wait_until(receiver_progress, SHMEM_CMP_GT, round - RING_DIFF + SHMEM_SYNC_VALUE);
        block_size_round = block_sizes + (round % RING_DIFF);

        // TODO: fix -> shmem_size_p(block_size_round, nbytes_round + 1 + SHMEM_SYNC_VALUE, send_to_pe);
        shmem_size_atomic_add(block_size_round, nbytes_round + 1 + SHMEM_SYNC_VALUE, send_to_pe);

        /* If writing block 0, reset offset to 0 */
        block_offset = (me_as + round + 1 == PE_size) ? 0 : block_offset + nbytes_round;

        /* Wait to receive the data in this round */
        shmem_size_wait_until(block_size_round, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
        nbytes_round = *block_size_round - 1 - SHMEM_SYNC_VALUE;

        shmem_size_put(block_size_round, (const size_t *) &SYNC_VALUE, 1, me);
        shmem_size_wait_until(block_size_round, SHMEM_CMP_EQ, SYNC_VALUE);

        shmem_long_atomic_inc(receiver_progress, recv_from_pe);
    }

    /* Must be atomic fadd because there may be some PE that did not finish with sends */
    shmem_long_atomic_add(receiver_progress, -round, me);
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

/* @formatter:on */
