#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_BARRIER_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_BARRIER_H

#include <shmem.h>

#define SHCOLL_BARRIER_SYNC_SIZE SHMEM_BARRIER_SYNC_SIZE

#define SHCOLL_SYNC_VALUE SHMEM_SYNC_VALUE


void shcoll_set_tree_degree(int tree_degree);

#define SHCOLL_BARRIER_DECLARATION(_name)                                       \
    void shcoll_##_name##_barrier(int PE_start, int logPE_stride,               \
                                   int PE_size, long *pSync);                   \
                                                                                \
    void shcoll_##_name##_barrier_all();

SHCOLL_BARRIER_DECLARATION(linear)
SHCOLL_BARRIER_DECLARATION(complete_tree)
SHCOLL_BARRIER_DECLARATION(binomial_tree)
SHCOLL_BARRIER_DECLARATION(dissemination)


#define SHCOLL_SYNC_DECLARATION(_name)                                          \
    void shcoll_##_name##_sync(int PE_start, int logPE_stride,                  \
                               int PE_size, long *pSync);                       \
                                                                                \
    void shcoll_##_name##_sync_all();

SHCOLL_SYNC_DECLARATION(linear)
SHCOLL_SYNC_DECLARATION(complete_tree)
SHCOLL_SYNC_DECLARATION(binomial_tree)
SHCOLL_SYNC_DECLARATION(dissemination)

#endif /* OPENSHMEM_COLLECTIVE_ROUTINES_BARRIER_H */
