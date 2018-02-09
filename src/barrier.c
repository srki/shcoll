//
// Created by Srđan Milaković on 2/8/18.
//

#include "barrier.h"

#include <shmem.h>
#include "util/trees.h"

static int _tree_degree = 2;

/*
 * Linear barrier implementation
 */

inline static void _barrier_sync_helper_linear(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    if (start == me) {
        const long npokes = size - 1;
        long poke = SHMEM_SYNC_VALUE + 1;

        /* wait for the rest of the AS to poke me */
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes);
        *pSync = SHMEM_SYNC_VALUE;

        /* send acks out */
        int pe = start + stride;
        for (int i = 1; i < size; i += 1) {
            shmem_long_put(pSync, &poke, 1, pe);
            pe += stride;
        }
    } else {
        /* poke root */
        shmem_long_atomic_inc(pSync, start);

        /* get ack */
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
        *pSync = SHMEM_SYNC_VALUE;
    }
}

/*
 * Complete tree barrier implementation
 */

inline static void _barrier_sync_helper_complete_tree(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    // Get my index in the active set
    const int me_as = (me - start) / stride;

    // Get node info
    node_info_complete_t node;
    get_node_info_complete(size, _tree_degree, me_as, &node);

    // Wait for pokes from the children
    long npokes = node.children_num;
    if (npokes != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes);
    }

    if (node.parent != -1) {
        // Poke the parent exists
        shmem_long_atomic_inc(pSync, start + node.parent * stride);

        // Wait for the poke from parent
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes + 1);
    }

    // Clear pSync and poke the children
    *pSync = SHMEM_SYNC_VALUE;

    for (int child = node.children_begin; child != node.children_end; child++) {
        shmem_long_atomic_inc(pSync, start + child * stride);
    }
}

/*
 * Binomial tree barrier implementation
 */

inline static void _barrier_sync_helper_binomial_tree(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    // Get my index in the active set
    const int me_as = (me - start) / stride;

    // Get node info
    node_info_binomial_t node; // TODO: try static
    get_node_info_binomial(size, me_as, &node);

    // Wait for pokes from the children
    long npokes = node.children_num;
    if (npokes != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes);
    }

    if (node.parent != -1) {
        // Poke the parent exists
        shmem_long_atomic_inc(pSync, start + node.parent * stride);

        // Wait for the poke from parent
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes + 1);
    }

    // Clear pSync and poke the children
    *pSync = SHMEM_SYNC_VALUE;

    for (int i = 0; i < node.children_num; i++) {
        shmem_long_atomic_inc(pSync, start + node.children[i] * stride);
    }
}

/*
 * Dissemination barrier implementation
 */

inline static void _barrier_sync_helper_dissemination(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    // Calculate my index in the active set
    const int me_as = (me - start) / stride;

    for (int round = 0, distance = 1; distance < size; round++, distance <<= 1) {
        int target_as = (me_as + distance) % size;

        // Poke the target for the current round
        shmem_long_atomic_inc(&pSync[round], start + target_as * stride);

        // Wait until poked in this round
        shmem_long_wait_until(&pSync[round], SHMEM_CMP_NE, SHMEM_SYNC_VALUE);

        // Reset pSync element, fadd is used instead of add because we have to
        // be sure that reset happens before next invocation of barrier
        shmem_long_atomic_fetch_add(&pSync[round], -1, me);
    }
}

void shcoll_set_tree_degree(int tree_degree) {
    _tree_degree = tree_degree;
}

static long barrier_all_pSync[SHCOLL_BARRIER_SYNC_SIZE] = {SHCOLL_SYNC_VALUE};
static long sync_all_pSync[SHCOLL_BARRIER_SYNC_SIZE] = {SHCOLL_SYNC_VALUE};


#define SHCOLL_BARRIER_DEFINITION(_name, _helper)                               \
    void shcoll_##_name##_barrier(int PE_start, int logPE_stride,               \
                                  int PE_size, long *pSync) {                   \
        _helper(PE_start, logPE_stride, PE_size, pSync);                        \
        shmem_quiet();                                                          \
    }                                                                           \
                                                                                \
    void shcoll_##_name##_barrier_all() {                                       \
        _helper(0, 0, shmem_n_pes(), barrier_all_pSync);                        \
        shmem_quiet();                                                          \
    }

SHCOLL_BARRIER_DEFINITION(linear, _barrier_sync_helper_linear)
SHCOLL_BARRIER_DEFINITION(complete_tree, _barrier_sync_helper_complete_tree)
SHCOLL_BARRIER_DEFINITION(binomial_tree, _barrier_sync_helper_binomial_tree)
SHCOLL_BARRIER_DEFINITION(dissemination, _barrier_sync_helper_dissemination)


#define SHCOLL_SYNC_DEFINITION(_name, _helper)                                  \
    void shcoll_##_name##_sync(int PE_start, int logPE_stride,                  \
                               int PE_size, long *pSync) {                      \
        _helper(PE_start, logPE_stride, PE_size, pSync);                        \
        /* TODO: memory fence */                                                \
    }                                                                           \
                                                                                \
    void shcoll_##_name##_sync_all() {                                          \
        _helper(0, 0, shmem_n_pes(), sync_all_pSync);                           \
        /* TODO: memory fence */                                                \
    }

SHCOLL_SYNC_DEFINITION(linear, _barrier_sync_helper_linear)
SHCOLL_SYNC_DEFINITION(complete_tree, _barrier_sync_helper_complete_tree)
SHCOLL_SYNC_DEFINITION(binomial_tree, _barrier_sync_helper_binomial_tree)
SHCOLL_SYNC_DEFINITION(dissemination, _barrier_sync_helper_dissemination)