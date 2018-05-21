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


SHCOLL_ALLTOALL_DECLARATION(loop, 32)
SHCOLL_ALLTOALL_DECLARATION(loop, 64)

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_ALLTOALL_H
