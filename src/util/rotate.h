/*
 * For license: see LICENSE file at top-level
 */

#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_ROTATE_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_ROTATE_H

#include <stddef.h>

void rotate_inplace(char *arr, size_t size, size_t dist);

void rotate(char *arr, size_t size, size_t dist);

#endif //OPENSHMEM_COLLECTIVE_ROUTINES_ROTATE_H
