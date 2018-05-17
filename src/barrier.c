#include "barrier.h"
#include <shmem.h>
#include "util/trees.h"

static int tree_degree_barrier = 2;

void shcoll_set_tree_degree(int tree_degree) {
    tree_degree_barrier = tree_degree;
}

/*
 * Linear barrier implementation
 */

inline static void barrier_sync_helper_linear(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    const long npokes = size - 1;
    long poke = SHMEM_SYNC_VALUE + 1;

    int i;
    int pe;

    if (start == me) {
        /* wait for the rest of the AS to poke me */
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes);
        *pSync = SHMEM_SYNC_VALUE;

        /* send acks out */
        pe = start + stride;
        for (i = 1; i < size; i += 1) {
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

inline static void barrier_sync_helper_complete_tree(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    int child;
    long npokes;
    node_info_complete_t node;

    /* Get my index in the active set */
    const int me_as = (me - start) / stride;

    /* Get node info */
    get_node_info_complete(size, tree_degree_barrier, me_as, &node);

    /* Wait for pokes from the children */
    npokes = node.children_num;
    if (npokes != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes);
    }

    if (node.parent != -1) {
        /* Poke the parent exists */
        shmem_long_atomic_inc(pSync, start + node.parent * stride);

        /* Wait for the poke from parent */
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes + 1);
    }

    /* Clear pSync and poke the children */
    *pSync = SHMEM_SYNC_VALUE;

    for (child = node.children_begin; child != node.children_end; child++) {
        shmem_long_atomic_inc(pSync, start + child * stride);
    }
}

/*
 * Binomial tree barrier implementation
 */

inline static void barrier_sync_helper_binomial_tree(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    int i;
    long npokes;
    node_info_binomial_t node; /* TODO: try static */

    /* Get my index in the active set */
    const int me_as = (me - start) / stride;

    /* Get node info */
    get_node_info_binomial(size, me_as, &node);

    /* Wait for pokes from the children */
    npokes = node.children_num;
    if (npokes != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes);
    }

    if (node.parent != -1) {
        /* Poke the parent exists */
        shmem_long_atomic_inc(pSync, start + node.parent * stride);

        /* Wait for the poke from parent */
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + npokes + 1);
    }

    /* Clear pSync and poke the children */
    *pSync = SHMEM_SYNC_VALUE;

    for (i = 0; i < node.children_num; i++) {
        shmem_long_atomic_inc(pSync, start + node.children[i] * stride);
    }
}

/*
 * Dissemination barrier implementation
 */

inline static void barrier_sync_helper_dissemination(int start, int log2stride, int size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << log2stride;

    int round;
    int distance;
    int target_as;

    /* Calculate my index in the active set */
    const int me_as = (me - start) / stride;

    for (round = 0, distance = 1; distance < size; round++, distance <<= 1) {
        target_as = (me_as + distance) % size;

        /* Poke the target for the current round */
        shmem_long_atomic_inc(&pSync[round], start + target_as * stride);

        /* Wait until poked in this round */
        shmem_long_wait_until(&pSync[round], SHMEM_CMP_NE, SHMEM_SYNC_VALUE);

        /* Reset pSync element, fadd is used instead of add because we have to
           be sure that reset happens before next invocation of barrier */
        shmem_long_atomic_fetch_add(&pSync[round], -1, me);
    }
}


static long barrier_all_pSync[SHCOLL_BARRIER_SYNC_SIZE] = {SHCOLL_SYNC_VALUE};
static long sync_all_pSync[SHCOLL_BARRIER_SYNC_SIZE] = {SHCOLL_SYNC_VALUE};


#define SHCOLL_BARRIER_DEFINITION(_name, _helper)                               \
    void shcoll_##_name##_barrier(int PE_start, int logPE_stride,               \
                                  int PE_size, long *pSync) {                   \
        shmem_quiet();                                                          \
        _helper(PE_start, logPE_stride, PE_size, pSync);                        \
    }                                                                           \
                                                                                \
    void shcoll_##_name##_barrier_all() {                                       \
        shmem_quiet();                                                          \
        _helper(0, 0, shmem_n_pes(), barrier_all_pSync);                        \
    }

SHCOLL_BARRIER_DEFINITION(linear, barrier_sync_helper_linear)
SHCOLL_BARRIER_DEFINITION(complete_tree, barrier_sync_helper_complete_tree)
SHCOLL_BARRIER_DEFINITION(binomial_tree, barrier_sync_helper_binomial_tree)
SHCOLL_BARRIER_DEFINITION(dissemination, barrier_sync_helper_dissemination)


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

SHCOLL_SYNC_DEFINITION(linear, barrier_sync_helper_linear)
SHCOLL_SYNC_DEFINITION(complete_tree, barrier_sync_helper_complete_tree)
SHCOLL_SYNC_DEFINITION(binomial_tree, barrier_sync_helper_binomial_tree)
SHCOLL_SYNC_DEFINITION(dissemination, barrier_sync_helper_dissemination)