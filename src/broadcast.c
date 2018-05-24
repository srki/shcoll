#include <stdio.h>
#include "broadcast.h"
#include "util/trees.h"

static int tree_degree_broadcast = 2;

void shcoll_set_broadcast_tree_degree(int tree_degree) {
    tree_degree_broadcast = tree_degree;
}


inline static void broadcast_helper_linear(void *target, const void *source, size_t nbytes, int PE_root,
                                           int PE_start, int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int root = (PE_root * stride) + PE_start;
    const int me = shmem_my_pe();

    shmem_barrier(PE_start, logPE_stride, PE_size, pSync); /* TODO: use shcoll barrier? */
    if (me != root) {
        shmem_char_get(target, source, nbytes, root);
    }
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync);
}

inline static void broadcast_helper_complete_tree(void *target, const void *source, size_t nbytes, int PE_root,
                                                  int PE_start, int logPE_stride, int PE_size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << logPE_stride;

    int parent;
    int child;
    int dst;
    node_info_complete_t node;

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    /* Get my index after tree root and broadcast root swap */
    /* TODO: remove if */
    if (me_as == PE_root) {
        me_as = 0;
    } else if (me_as == 0) {
        me_as = PE_root;
    }

    /* Get information about children */
    get_node_info_complete(PE_size, tree_degree_broadcast, me_as, &node);

    /* Wait for the data form the parent */
    if (me_as != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
        source = target;

        /* Send ack */
        parent = node.parent;
        /* TODO: remove if */
        if (parent == PE_root) {
            parent = 0;
        } else if (parent == 0) {
            parent = PE_root;
        }
        shmem_long_atomic_inc(pSync, PE_start + parent * stride);
    }

    /* Send data to children */
    if (node.children_num != 0) {
        for (child = node.children_begin; child != node.children_end; child++) {
            dst = PE_start + (child == PE_root ? 0 : child) * stride;
            shmem_char_put_nbi(target, source, nbytes, dst);
        }

        shmem_fence();

        for (child = node.children_begin; child != node.children_end; child++) {
            dst = PE_start + (child == PE_root ? 0 : child) * stride;
            shmem_long_atomic_inc(pSync, dst);
        }


        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + node.children_num + (me_as == 0 ? 0 : 1));
    }

    *pSync = SHMEM_SYNC_VALUE;
}

inline static void broadcast_helper_binomial_tree(void *target, const void *source, size_t nbytes, int PE_root,
                                                  int PE_start, int logPE_stride, int PE_size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << logPE_stride;

    int i;
    int parent;
    int dst;
    node_info_binomial_t node;

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    /* Get my index after tree root and broadcast root swap */
    if (me_as == PE_root) {
        me_as = 0;
    } else if (me_as == 0) {
        me_as = PE_root;
    }

    /* Get information about children */
    get_node_info_binomial(PE_size, me_as, &node);

    /* Wait for the data form the parent */
    if (me_as != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
        source = target;
        /* Send ack */
        parent = node.parent;
        if (parent == PE_root) {
            parent = 0;
        } else if (parent == 0) {
            parent = PE_root;
        }
        shmem_long_atomic_inc(pSync, PE_start + parent * stride);
    }

    /* TODO: try inc-get-inc instead of put and inc (probably not necessary because binomial is used for short msgs) */
    /* Send data to children */
    if (node.children_num != 0) {
        for (i = 0; i < node.children_num; i++) {
            dst = PE_start + (node.children[i] == PE_root ? 0 : node.children[i]) * stride;
            shmem_char_put_nbi(target, source, nbytes, dst);
            shmem_fence();
            shmem_long_atomic_inc(pSync, dst);
        }

        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + node.children_num + (me_as == 0 ? 0 : 1));
    }

    *pSync = SHMEM_SYNC_VALUE;
}


inline static void broadcast_helper_scatter_collect(void *target, const void *source, size_t nbytes, int PE_root,
                                                    int PE_start, int logPE_stride, int PE_size, long *pSync) {
    /* TODO: Optimize cases where data_start == data_end (block has size 0) */

    const int me = shmem_my_pe();
    const int stride = 1 << logPE_stride;

    int root_as = (PE_root - PE_start) / stride;

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    /* Shift me_as so that me_as for PE_root is 0 */
    me_as = (me_as - root_as + PE_size) % PE_size;

    /* The number of received blocks (scatter + collect) */
    int total_received = me_as == 0 ? PE_size : 0;

    int target_pe;
    int next_as = (me_as + 1) % PE_size;
    int next_pe = PE_start + (root_as + next_as) % PE_size * stride;

    /* The index of the block that should be send to next_pe */
    int next_block = me_as;

    /* The number of blocks that next received */
    int next_pe_nblocks = next_as == 0 ? PE_size : 0;

    int left = 0;
    int right = PE_size;
    int mid;
    int dist;

    size_t data_start;
    size_t data_end;

    /* Used in the collect part to wait for new blocks */
    long ring_received = SHMEM_SYNC_VALUE;

    if (me_as != 0) {
        source = target;
    }

    /* Scatter data to other PEs using binomial tree */
    while (right - left > 1) {
        /* dist = ceil((right - let) / 2) */
        dist = ((right - left) >> 1) + ((right - left) & 0x1);
        mid = left + dist;


        /* Send (right - mid) elements starting with mid to pe + dist */
        if (me_as == left && me_as + dist < right) {
            data_start = (mid * nbytes + PE_size - 1) / PE_size;
            data_end = (right * nbytes + PE_size - 1) / PE_size;
            target_pe = PE_start + (root_as + me_as + dist) % PE_size * stride;

            shmem_putmem(((char *) target) + data_start, ((char *) source) + data_start,
                         data_end - data_start, target_pe);
            shmem_fence();
            shmem_long_atomic_inc(pSync, target_pe);
        }

        /* Send (right - mid) elements starting with mid from (me_as - dist) */
        if (me_as - dist == left) {
            shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
            total_received = right - mid;
        }

        if (next_as - dist == left) {
            next_pe_nblocks = right - mid;
        }

        if (me_as < mid) {
            right = mid;
        } else {
            left = mid;
        }
    }


    /* Do collect using (modified) ring algorithm */
    while (next_pe_nblocks != PE_size) {
        data_start = (next_block * nbytes + PE_size - 1) / PE_size;
        data_end = ((next_block + 1) * nbytes + PE_size - 1) / PE_size;

        shmem_putmem(((char *) target) + data_start, ((char *) source) + data_start, data_end - data_start, next_pe);
        shmem_fence();
        shmem_long_atomic_inc(pSync + 1, next_pe);


        next_pe_nblocks++;
        next_block = (next_block - 1 + PE_size) % PE_size;

        /* If we did not received all blocks, we must wait for the next block we want to send*/
        if (total_received != PE_size) {
            shmem_long_wait_until(pSync + 1, SHMEM_CMP_GT, ring_received);
            ring_received++;
            total_received++;
        }
    }

    while (total_received != PE_size) {
        shmem_long_wait_until(pSync + 1, SHMEM_CMP_GT, ring_received);
        ring_received++;
        total_received++;
    }

    // TODO: maybe only one pSync is enough
    pSync[0] = SHMEM_SYNC_VALUE;
    pSync[1] = SHMEM_SYNC_VALUE;
}


#define SHCOLL_BROADCAST_DEFINITION(_name, _size)                                           \
    void shcoll_broadcast##_size##_##_name(void *dest, const void *source,                  \
                                           size_t nelems, int PE_root, int PE_start,        \
                                           int logPE_stride, int PE_size, long *pSync) {    \
        broadcast_helper_##_name(dest, source, (_size) / CHAR_BIT * nelems,                 \
                PE_root, PE_start, logPE_stride, PE_size, pSync);                           \
    }                                                                                       \

/* @formatter:off */

SHCOLL_BROADCAST_DEFINITION(linear, 8)
SHCOLL_BROADCAST_DEFINITION(linear, 16)
SHCOLL_BROADCAST_DEFINITION(linear, 32)
SHCOLL_BROADCAST_DEFINITION(linear, 64)

SHCOLL_BROADCAST_DEFINITION(complete_tree, 8)
SHCOLL_BROADCAST_DEFINITION(complete_tree, 16)
SHCOLL_BROADCAST_DEFINITION(complete_tree, 32)
SHCOLL_BROADCAST_DEFINITION(complete_tree, 64)

SHCOLL_BROADCAST_DEFINITION(binomial_tree, 8)
SHCOLL_BROADCAST_DEFINITION(binomial_tree, 16)
SHCOLL_BROADCAST_DEFINITION(binomial_tree, 32)
SHCOLL_BROADCAST_DEFINITION(binomial_tree, 64)

SHCOLL_BROADCAST_DEFINITION(scatter_collect, 8)
SHCOLL_BROADCAST_DEFINITION(scatter_collect, 16)
SHCOLL_BROADCAST_DEFINITION(scatter_collect, 32)
SHCOLL_BROADCAST_DEFINITION(scatter_collect, 64)

/* @formatter:on */