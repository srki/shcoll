//
// Created by Srdan Milakovic on 5/17/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_FCOLLECT_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_FCOLLECT_H

#include <stddef.h>
#include "global.h"

#define SHCOLL_FCOLLECT_DECLARATION(_name, _size)                                                       \
    void shcoll_fcollect##_size##_##_name(void *dest, const void *source, size_t nelems,                \
                                          int PE_start, int logPE_stride, int PE_size, long *pSync);    \


SHCOLL_FCOLLECT_DECLARATION(linear, 32)
SHCOLL_FCOLLECT_DECLARATION(linear, 64)

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_FCOLLECT_H
