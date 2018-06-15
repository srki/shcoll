//
// Created by Srdan Milakovic on 5/21/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALL_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALL_H

#include "global.h"

#define SHCOLL_ALLTOALL_DECLARATION(_name, _size)                                           \
    void shcoll_alltoall##_size##_##_name(void *dest, const void *source, size_t nelems,    \
                                          int PE_start, int logPE_stride, int PE_size,      \
                                          long *pSync);                                     \

// @formatter:off

SHCOLL_ALLTOALL_DECLARATION(shift_exchange_barrier, 32)
SHCOLL_ALLTOALL_DECLARATION(shift_exchange_barrier, 64)

SHCOLL_ALLTOALL_DECLARATION(shift_exchange_counter, 32)
SHCOLL_ALLTOALL_DECLARATION(shift_exchange_counter, 64)

SHCOLL_ALLTOALL_DECLARATION(shift_exchange_signal, 32)
SHCOLL_ALLTOALL_DECLARATION(shift_exchange_signal, 64)


SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, 64)


SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, 64)

// @formatter:on

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALL_H
