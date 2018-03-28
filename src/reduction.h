#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_REDUCTION_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_REDUCTION_H

/* TODO: add SCHOLL_REDUCE_MIN_WRKDATA_SIZE */

void int_sum_to_all_helper_linear(int *dest, const int *source, int nreduce, int PE_start,
                                  int logPE_stride, int PE_size, int *pWrk, long *pSync);

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_REDUCTION_H
