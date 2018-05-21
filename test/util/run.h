//
// Created by Srdan Milakovic on 5/21/18.
//

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_RUN_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_RUN_H

#include <shmem.h>
#include "debug.h"

#define RUN(_func, _name, ...)                                                              \
    if (shmem_my_pe() == 0) {                                                               \
        gprintf(#_name ": %lf\n", test_##_func(shcoll_##_func##_##_name, __VA_ARGS__));     \
    } else {                                                                                \
        test_##_func(shcoll_##_func##_##_name, __VA_ARGS__);                                \
    }                                                                                       \


#endif //OPENSHMEM_COLLECTIVE_ROUTINES_RUN_H
