//
// Created by Srdan Milakovic on 5/23/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_WRAPER_1_3_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_WRAPER_1_3_H

#if SHMEM_MAJOR_VERSION == 1 && SHMEM_MINOR_VERSION == 3
#define shmem_sync_all shmem_barrier_all
#endif

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_WRAPER_1_3_H
