//
// Created by Srdan Milakovic on 5/17/18.
//

#include "fcollect.h"
#include "barrier.h"
#include "broadcast.h"
#include "../test/util/debug.h"
#include <limits.h>
#include <string.h>

inline static void fcollect_helper_linear(void *dest, const void *source, size_t nbytes, int PE_start,
                                          int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    int me_as = (me - PE_start) / stride;

    shcoll_linear_barrier_all();
    if (me_as != 0) {
        shmem_char_put(dest + me_as * nbytes, source, nbytes, PE_start);
    } else {
        memcpy(dest, source, nbytes);
    }
    shcoll_linear_barrier_all();

    shcoll_linear_broadcast8(dest, dest, nbytes * shmem_n_pes(), 0, PE_start, logPE_stride, PE_size, pSync);
}




#define SHCOLL_FCOLLECT_DEFINITION(_name, _size)                                                        \
    void shcoll_fcollect##_size##_##_name(void *dest, const void *source, size_t nelems,                \
                                          int PE_start, int logPE_stride, int PE_size, long *pSync) {   \
        fcollect_helper_##_name(dest, source, (_size) / CHAR_BIT * nelems,                              \
                               PE_start, logPE_stride, PE_size, pSync);                                 \
}                                                                                                       \


SHCOLL_FCOLLECT_DEFINITION(linear, 32)

SHCOLL_FCOLLECT_DEFINITION(linear, 64)