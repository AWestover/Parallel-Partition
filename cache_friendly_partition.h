
#include <stdint.h>
#ifndef CACHE_FRIENDLY_PARTITION_H
#define CACHE_FRIENDLY_PARTITION_H
int64_t serialPartitioner(int64_t* A, int64_t n, int64_t pivotVal);
int64_t groupedPartition(int64_t* A, int64_t N0, int64_t pivotVal);
void parallel_quicksort_cache_friendly (int64_t* array, uint64_t num_elts);
#endif
