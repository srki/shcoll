#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_BROADCAST_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_BROADCAST_H

#include <stddef.h>
#include "global.h"

void shcoll_set_broadcast_tree_degree(int tree_degree);

#define SHCOLL_BROADCAST_DECLARATION(_name, _size)                              \
    void shcoll_##_name##_broadcast##_size(void *dest, const void *source,      \
                                    size_t nelems, int PE_root, int PE_start,   \
                                    int logPE_stride, int PE_size,              \
                                    long *pSync);

SHCOLL_BROADCAST_DECLARATION(linear, 8)
SHCOLL_BROADCAST_DECLARATION(linear, 16)
SHCOLL_BROADCAST_DECLARATION(linear, 32)
SHCOLL_BROADCAST_DECLARATION(linear, 64)

SHCOLL_BROADCAST_DECLARATION(complete_tree, 8)
SHCOLL_BROADCAST_DECLARATION(complete_tree, 16)
SHCOLL_BROADCAST_DECLARATION(complete_tree, 32)
SHCOLL_BROADCAST_DECLARATION(complete_tree, 64)

SHCOLL_BROADCAST_DECLARATION(binomial_tree, 8)
SHCOLL_BROADCAST_DECLARATION(binomial_tree, 16)
SHCOLL_BROADCAST_DECLARATION(binomial_tree, 32)
SHCOLL_BROADCAST_DECLARATION(binomial_tree, 64)

#endif /* OPENSHMEM_COLLECTIVE_ROUTINES_BROADCAST_H */
