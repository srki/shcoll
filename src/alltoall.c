//
// Created by Srdan Milakovic on 5/21/18.
//

#include "alltoall.h"
#include "barrier.h"
#include <string.h>
#include <limits.h>

inline static void alltoall_helper_loop(void *dest, const void *source, size_t nelems, int PE_start,
                                          int logPE_stride, int PE_size, long *pSync) {
    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();

    /* Get my index in the active set */
    const int me_as = (me - PE_start) / stride;

    void *const dest_ptr = ((uint8_t *) dest) + me * nelems;
    void const *source_ptr = ((uint8_t *) source) + me * nelems;

    int i;
    int peer_as;

    memcpy(dest_ptr, source_ptr, nelems);

    for (i = 1; i < PE_size; i++) {
        peer_as = (me_as + i) % PE_size;
        source_ptr = ((uint8_t *) source) + peer_as * nelems;

        shmem_putmem(dest_ptr, source_ptr, nelems, PE_start + peer_as * stride);
    }

    /* TODO: change to auto shcoll barrier */
    shcoll_binomial_tree_barrier(PE_start, logPE_stride, PE_size, pSync);
}


#define SHCOLL_ALLTOALL_DEFINITION(_name, _size)                                                        \
    void shcoll_alltoall##_size##_##_name(void *dest, const void *source, size_t nelems, int PE_start,  \
                                       int logPE_stride, int PE_size, long *pSync) {                    \
        alltoall_helper_##_name(dest, source, (_size) / (CHAR_BIT) * nelems,                            \
                                PE_start, logPE_stride, PE_size, pSync);                                \
}                                                                                                       \


SHCOLL_ALLTOALL_DEFINITION(loop, 32)

SHCOLL_ALLTOALL_DEFINITION(loop, 64)