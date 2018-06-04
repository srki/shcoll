//
// Created by Srdan Milakovic on 5/21/18.
//

#include "alltoalls.h"
#include "barrier.h"
#include <string.h>
#include <limits.h>


#define SHCOLL_ALLTOALLS_DEFINITION(_name, _size)                                                                   \
void shcoll_alltoalls##_size##_##_name(void *dest, const void *source, ptrdiff_t dst,                               \
                                       ptrdiff_t sst, size_t nelems, int PE_start,                                  \
                                       int logPE_stride, int PE_size, long *pSync) {                                \
    const int stride = 1 << logPE_stride;                                                                           \
    const int me = shmem_my_pe();                                                                                   \
                                                                                                                    \
    /* Get my index in the active set */                                                                            \
    const int me_as = (me - PE_start) / stride;                                                                     \
                                                                                                                    \
    void *const dest_ptr = ((uint8_t *) dest) + me * dst * nelems * ((_size) / CHAR_BIT);                           \
    void const *source_ptr = ((uint8_t *) source) + me * sst * nelems * ((_size) / CHAR_BIT);                       \
                                                                                                                    \
    int i;                                                                                                          \
    int peer_as;                                                                                                    \
                                                                                                                    \
    /* TODO: copy in a loop */                                                                                      \
    /* memcpy(dest_ptr, source_ptr, nelems); */                                                                     \
                                                                                                                    \
    for (i = 0; i < PE_size; i++) {                                                                                 \
        peer_as = (me_as + i) % PE_size;                                                                            \
        source_ptr = ((uint8_t *) source) + peer_as * sst * nelems * ((_size) / CHAR_BIT);                          \
                                                                                                                    \
        shmem_iput##_size(dest_ptr, source_ptr, dst, sst, nelems, PE_start + peer_as * stride);   \
    }                                                                                                               \
                                                                                                                    \
    /* TODO: change to auto shcoll barrier */                                                                       \
    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);                                           \
}                                                                                                                   \



SHCOLL_ALLTOALLS_DEFINITION(loop, 32)

SHCOLL_ALLTOALLS_DEFINITION(loop, 64)