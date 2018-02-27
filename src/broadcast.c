#include "broadcast.h"
#include <shmem.h>
#include "util/trees.h"

static int _tree_degree_broadcast = 2;

void scholl_set_broadcast_tree_degree(int tree_degree) {
    _tree_degree_broadcast = tree_degree;
}


inline static void _broadcast_helper_linear(void *target, const void *source, size_t nbytes, int PE_root,
                                            int PE_start, int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int root = (PE_root * stride) + PE_start;
    const int me = shmem_my_pe();

    shmem_barrier(PE_start, logPE_stride, PE_size, pSync); /* TODO: use shcoll barrier? */
    if (me != root) {
        shmem_char_get(target, source, nbytes, root);
    }
    shmem_barrier(PE_start, logPE_stride, PE_size, pSync);
    shmem_quiet();
}

inline static void _broadcast_helper_complete_tree(void *target, const void *source, size_t nbytes, int PE_root,
                                                   int PE_start, int logPE_stride, int PE_size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << logPE_stride;

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
    node_info_complete_t node;
    get_node_info_complete(PE_size, _tree_degree_broadcast, me_as, &node);

    /* Wait for the data form the parent */
    if (me_as != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
        source = target;

        /* Send ack */
        int parent = node.parent;
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
        for (int child = node.children_begin; child != node.children_end; child++) {
            int dst = PE_start + (child == PE_root ? 0 : child) * stride;
            shmem_char_put_nbi(target, source, nbytes, dst);
        }

        shmem_fence();

        for (int child = node.children_begin; child != node.children_end; child++) {
            int dst = PE_start + (child == PE_root ? 0 : child) * stride;
            shmem_long_atomic_inc(pSync, dst);
        }


        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + node.children_num + (me_as == 0 ? 0 : 1));
    }

    *pSync = SHMEM_SYNC_VALUE;
}

inline static void _broadcast_helper_binomial_tree(void *target, const void *source, size_t nbytes, int PE_root,
                                              int PE_start, int logPE_stride, int PE_size, long *pSync) {
    const int me = shmem_my_pe();
    const int stride = 1 << logPE_stride;

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    /* Get my index after tree root and broadcast root swap */
    if (me_as == PE_root) {
        me_as = 0;
    } else if (me_as == 0) {
        me_as = PE_root;
    }

    /* Get information about children */
    node_info_binomial_t node;
    get_node_info_binomial(PE_size, me_as, &node);

    /* Wait for the data form the parent */
    if (me_as != 0) {
        shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHMEM_SYNC_VALUE);
        source = target;
        /* Send ack */
        int parent = node.parent;
        if (parent == PE_root) {
            parent = 0;
        } else if (parent == 0) {
            parent = PE_root;
        }
        shmem_long_atomic_inc(pSync, PE_start + parent * stride);
    }

    /* Send data to children */
    if (node.children_num != 0) {
        for (int i = 0; i < node.children_num; i++) {
            int dst = PE_start + (node.children[i] == PE_root ? 0 : node.children[i]) * stride;
            shmem_char_put_nbi(target, source, nbytes, dst);
            shmem_fence();
            shmem_long_atomic_inc(pSync, dst);
        }

        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHMEM_SYNC_VALUE + node.children_num + (me_as == 0 ? 0 : 1));
    }

    *pSync = SHMEM_SYNC_VALUE;
}



#define SHCOLL_BROADCAST_DEFINITION(_name, _helper, _size)                      \
    void shcoll_##_name##_broadcast##_size(void *dest, const void *source,      \
                                    size_t nelems, int PE_root, int PE_start,   \
                                    int logPE_stride, int PE_size,              \
                                    long *pSync) {                              \
        _helper(dest, source, (_size) / CHAR_BIT * nelems,                      \
                PE_root, PE_start, logPE_stride, PE_size, pSync);               \
    }                                                                           \

SHCOLL_BROADCAST_DEFINITION(linear, _broadcast_helper_linear, 32)
SHCOLL_BROADCAST_DEFINITION(linear, _broadcast_helper_linear, 64)

SHCOLL_BROADCAST_DEFINITION(complete_tree, _broadcast_helper_complete_tree, 32)
SHCOLL_BROADCAST_DEFINITION(complete_tree, _broadcast_helper_complete_tree, 64)

SHCOLL_BROADCAST_DEFINITION(binomial_tree, _broadcast_helper_binomial_tree, 32)
SHCOLL_BROADCAST_DEFINITION(binomial_tree, _broadcast_helper_binomial_tree, 64)
