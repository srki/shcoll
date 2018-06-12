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

/* @formatter:off */

SHCOLL_COLLECT_DECLARATION(linear, 32)
SHCOLL_COLLECT_DECLARATION(linear, 64)

SHCOLL_COLLECT_DECLARATION(all_linear, 32)
SHCOLL_COLLECT_DECLARATION(all_linear, 64)

SHCOLL_COLLECT_DECLARATION(all_linear1, 32)
SHCOLL_COLLECT_DECLARATION(all_linear1, 64)

SHCOLL_COLLECT_DECLARATION(ring, 32)
SHCOLL_COLLECT_DECLARATION(ring, 64)

SHCOLL_COLLECT_DECLARATION(bruck, 32)
SHCOLL_COLLECT_DECLARATION(bruck, 64)

SHCOLL_COLLECT_DECLARATION(bruck_no_rotate, 32)
SHCOLL_COLLECT_DECLARATION(bruck_no_rotate, 64)

/* @formatter:on */


#endif //OPENSHMEM_COLLECTIVE_ROUTINES_COLLECT_H
