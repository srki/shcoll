//
// Created by Srdan Milakovic on 5/15/18.
//

#include "collect.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "barrier.h"
#include "broadcast.h"

inline static void collect_helper_linear(void *dest, const void *source, size_t nbytes, int PE_start,
                                         int logPE_stride, int PE_size, long *pSync) {
    /* pSync[0] is used for barrier
     * pSync[1] is used for broadcast
     * next sizeof(size_t) bytes are used for the offset */

    const int stride = 1 << logPE_stride;
    const int me = shmem_my_pe();
    const int me_as = (me - PE_start) / stride;
    size_t *offset = (size_t *) (pSync + 2);
    int i;

    /* set offset to 0 */
    offset[0] = 0;
    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);

    if (me_as == 0) {
        shmem_size_atomic_add(offset, nbytes + 1, me + stride);
        memcpy(dest, source, nbytes);

        /* Wait for the full array size and notify everybody */
        shmem_size_wait_until(offset, SHMEM_CMP_NE, 0);

        /* Send offset to everybody */
        for (i = 1; i < PE_size; i++) {
            //shmem_size_atomic_set(offset, *offset, PE_start + i * stride);
            shmem_size_put(offset, offset, 1, PE_start + i * stride);
        }
    } else {
        shmem_size_wait_until(offset, SHMEM_CMP_NE, 0);

        /* Send offset to the next PE, PE_start will contain full array size */
        shmem_size_atomic_add(offset, nbytes + *offset, PE_start + ((me_as + 1) % PE_size) * stride);

        /* Write data to PE 0 */
        shmem_char_put((char *) dest + *offset - 1, source, nbytes, PE_start);
    }

    /* Wait for all PEs to send the data to PE_start */
    //shmem_quiet();
    shcoll_linear_barrier(PE_start, logPE_stride, PE_size, pSync);
    //shmem_barrier(PE_start, logPE_stride, PE_size, pSync);

    //shcoll_linear_broadcast64(offset, offset, 1, PE_start, PE_start, logPE_stride, PE_size, pSync + 4);

    //fprintf(stderr, "%d %zu\n", shmem_my_pe(), shmem_size_atomic_fetch(offset, me));
    //while (*offset != 1201);
    //fprintf(stderr, "%d %zu\n", shmem_my_pe(), *offset);

    shcoll_linear_broadcast8(dest, dest, *offset - 1, PE_start, PE_start, logPE_stride, PE_size, pSync + 1);
}

#define SHCOLL_COLLECT_DEFINITION(_name, _size)                                                         \
    void shcoll_collect##_size##_##_name(void *dest, const void *source, size_t nelems,                 \
                                         int PE_start, int logPE_stride, int PE_size, long *pSync) {    \
        collect_helper_##_name(dest, source, (_size) / CHAR_BIT * nelems,                               \
                               PE_start, logPE_stride, PE_size, pSync);                                 \
}                                                                                                       \


SHCOLL_COLLECT_DEFINITION(linear, 32)

SHCOLL_COLLECT_DEFINITION(linear, 64)


