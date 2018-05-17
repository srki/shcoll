//
// Created by Srdan Milakovic on 5/15/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_COLLECT_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_COLLECT_H

#include <stddef.h>
#include "global.h"

#define SHCOLL_COLLECT_DECLARATION(_name, _size)                                                    \
    void shcoll_collect##_size##_##_name(void *dest, const void *source, size_t nelems,             \
                                         int PE_start, int logPE_stride, int PE_size, long *pSync); \


SHCOLL_COLLECT_DECLARATION(linear, 32)
SHCOLL_COLLECT_DECLARATION(linear, 64)

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_COLLECT_H
