//
// Created by Srdan Milakovic on 5/17/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_DEBUG_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_DEBUG_H

#include "stdio.h"
#include <assert.h>

#define gprintf(...) fprintf(stderr, __VA_ARGS__)
#define PL gprintf("[%d]:%d\n", shmem_my_pe(), __LINE__);
#define PLI(A) gprintf("[%d]:%d %d\n", shmem_my_pe(), __LINE__, (A));
#define PLII(A, B) gprintf("[%d]:%d %d %d\n", shmem_my_pe(), __LINE__, (A), (B));

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_DEBUG_H
