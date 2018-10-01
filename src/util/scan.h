/*
 * For license: see LICENSE file at top-level
 */

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_SCAN_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_SCAN_H

#include <stddef.h>

/* TODO: move somewhere */
#define PREFIX_SUM_SYNC_SIZE 32

/* TODO: maybe use size_t *pWrk instead of pSync */
void exclusive_prefix_sum(size_t *dest, size_t value, int PE_start, int logPE_stride, int PE_size, long *pSync);

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_SCAN_H
