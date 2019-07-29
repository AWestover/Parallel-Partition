
#include <stdint.h>
#ifndef STRIDED_FRIENDLY_PARTITION_H
#define STRIDED_FRIENDLY_PARTITION_H
int64_t stridedPartition(int64_t* A, int64_t N0, int64_t pivotVal);
void parallel_quicksort_strided_partition (int64_t* array, uint64_t num_elts);
#endif
