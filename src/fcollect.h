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

/* @formatter:off */

SHCOLL_FCOLLECT_DECLARATION(linear, 32)
SHCOLL_FCOLLECT_DECLARATION(linear, 64)

SHCOLL_FCOLLECT_DECLARATION(rec_dbl, 32)
SHCOLL_FCOLLECT_DECLARATION(rec_dbl, 64)

SHCOLL_FCOLLECT_DECLARATION(ring, 32)
SHCOLL_FCOLLECT_DECLARATION(ring, 64)

SHCOLL_FCOLLECT_DECLARATION(bruck, 32)
SHCOLL_FCOLLECT_DECLARATION(bruck, 64)

SHCOLL_FCOLLECT_DECLARATION(neighbour_exchange, 32)
SHCOLL_FCOLLECT_DECLARATION(neighbour_exchange, 64)

/* @formatter:on */


#endif //OPENSHMEM_COLLECTIVE_ROUTINES_FCOLLECT_H
