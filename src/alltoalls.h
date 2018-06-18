//
// Created by Srdan Milakovic on 5/21/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALLS_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALLS_H

#include "global.h"

#define SHCOLL_ALLTOALLS_DECLARATION(_name, _size)                                          \
    void shcoll_alltoalls##_size##_##_name(void *dest, const void *source, ptrdiff_t dst,   \
                                           ptrdiff_t sst, size_t nelems, int PE_start,      \
                                           int logPE_stride, int PE_size, long *pSync);     \

// @formatter:of

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier, 64)

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter, 64)

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_barrier_nbi, 64)

SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(shift_exchange_counter_nbi, 64)


SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier_nbi, 64)

SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter_nbi, 64)


SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier_nbi, 64)

SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter_nbi, 32)
SHCOLL_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter_nbi, 64)

// @formatter:on

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALLS_H
