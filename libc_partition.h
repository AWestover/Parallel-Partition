/*
 * Libc Quicksort Partition.
 */

#include <cstdint>
#include "params.h"

#ifndef LIBC_PARTITION_H
#define LIBC_PARTITION_H
uint64_t libc_partition(int64_t *array, uint64_t n, int64_t pivot);
uint64_t libc_partition_strict(int64_t *array, uint64_t n, int64_t pivot);
void libc_quicksort (void *const pbase, size_t total_elems);
#endif
int64_t select_pivot(int64_t* array, uint64_t num_elts);
