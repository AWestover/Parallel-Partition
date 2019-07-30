
#include "params.h"
#include <stdio.h>
#include <stdint.h> // we are sorting int64_t
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include "libc_partition.h"
#include "cache_friendly_partition.h" // serialPartition, get_num_preds
using namespace std;

// NOTE: some functions were renamed to avoid naming collisions with functions in cache_friendly_partition.cpp
// this does not mean that they are different functions

struct Vs {
	int64_t vmin; 
	int64_t vmax;
};

/*
 * INPUT: 
 * 	startPi is the index of the first P_i that we are considering
 * 	numPs is the number of P_i that we are considering
 *	i.e. we are going to calculate min vy and max vy for P_startPi, P_startPi+1, ..., P_startPi+numPs -1
 *
 *	A the array
 *	pivotVal what we are partitioning with respect to
 *	b is the block size
 *	n is the size of A
 *	t is the offset determining the elts in each P_i
 * */
Vs serialStridedPartitions(int64_t startPi, int64_t numPs, int64_t *A, int64_t pivotVal, int64_t b, int64_t n, int64_t t){
	if(numPs == 1){
		// getting seg faults here !!!!
		int64_t low = startPi*b;
		int64_t blockBoundaryLow = low + b;
		int64_t blockBoundaryHigh = (((startPi*b + t*b * (((n/b)-startPi)/t)) <= n-b) ? (startPi*b + t*b * (((n/b)-startPi)/t)) : (startPi*b + t*b*( (((n/b)-startPi)/t) - 1 ))) - 1;
		int64_t high = blockBoundaryHigh + b;
		assert(high < n);
		while(low < high){
			while(A[low] <= pivotVal && low < high) { // we have to check low < high because otherwise [1 2 3 4 5] with pivot value 10 would cause a problem
				low++;
				if(low == blockBoundaryLow){
					blockBoundaryLow += t*b;
					low = blockBoundaryLow - b;
				}
			}
			while (A[high] > pivotVal && low < high){
				high--;
				if (high == blockBoundaryHigh){
					blockBoundaryHigh -= t*b;
					high = blockBoundaryHigh + b;
				}
			}
			// swap
			int64_t tmp = A[low];
			A[low] = A[high];
			A[high] = tmp;
		}
		if(A[low] <= pivotVal && low + (t-1)*b <= n){
			low++;
			if(low == blockBoundaryLow){
				blockBoundaryLow += t*b;
				low = blockBoundaryLow - b;
			}
		}
		Vs vscur;
		vscur.vmin = low;
		vscur.vmax = low;
		return vscur;
	}
	else{
		Vs vsleft = parallel_spawn serialStridedPartitions(startPi, numPs/2, A, pivotVal, b, n, t);
		Vs vsright = serialStridedPartitions(startPi + numPs/2, numPs/2+(numPs&1), A, pivotVal, b, n, t);
		parallel_sync;
		Vs vsaggregate;
		vsaggregate.vmin = std::min(vsleft.vmin, vsright.vmin);
		vsaggregate.vmax = std::max(vsleft.vmax, vsright.vmax);
		return vsaggregate;
	}
}

/*
 * INPUT: an array A of size n, the size n, and the pivotVal which the array will be partitioned with respect to
 * OUTPUT: the function partitions the array A in place and returns the number of predecessors in the array
 * */
int64_t stridedPartition(int64_t* A, int64_t n, int64_t pivotVal) {
	int64_t b = 1<<9;
	int64_t t = (int64_t)pow(n, 0.333333);

	if (n > 2*t*b){
		Vs vsfinal = serialStridedPartitions(0, t, A, pivotVal, b, n, t);
		// std::cout << "vmin: " << vsfinal.vmin << ", vmax: " << vsfinal.vmax << std::endl;
		for (int i = 0; i < vsfinal.vmin; i++) {
			if(A[i]>pivotVal){
				std::cout << i << std::endl;
				assert(false);
			}
		}
		int64_t num_preds = vsfinal.vmin + serialPartition(A+vsfinal.vmin, vsfinal.vmax - vsfinal.vmin, pivotVal);
		if(num_preds < n && A[num_preds] <= pivotVal){
			num_preds += 1;
		}
		return num_preds;
	}
	else{ // problem size is too small to merit a parallel alg
		return serialPartition(A, n, pivotVal);
	}
}

/*
 * Parallel quicksort implementation using our cache-friendly parallel
 * partition. This is called internally by the parallel_quicksort with
 * fewer arguments. Once problem-sizes get to size_cutoff or smaller,
 * we swap to serial.
 */
void parallel_quicksort_strided_partition (int64_t* array, uint64_t num_elts, uint64_t size_cutoff) {
  if (num_elts <= 1) return;
  if (num_elts <= size_cutoff) {
    libc_quicksort(array, num_elts);
  } else {
    int64_t pivot = select_pivot(array, num_elts);
    int64_t num_preds = stridedPartition(array, num_elts, pivot);

	assert(num_preds == get_num_preds(array, num_elts, pivot));
    // Now we handle an important edge case. If it turns out that the
    // vast majority (i.e. more than a constant fraction) of the
    // elements equal the pivot, then the partition done in the
    // previous step won't have been very useful. To detect that the
    // pivot was very common, we select a new pivot from the
    // predecessors P of the partition we just performed. If the new
    // pivot equals the old one, then we perform another partition on
    // P in which we partition on pivot - 1 (or for a more general
    // sort implementation, we would partition based on < pivot
    // instead of <= pivot). This places the elements equal to pivot
    // in their final destination. Folowing the convention used in
    // libc-qsort we use deterministic instead of random pivots; see
    // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.14.8162&rep=rep1&type=pdf
    int64_t second_pivot = array[num_preds / 2];
    if (second_pivot == pivot) {
      parallel_spawn parallel_quicksort_strided_partition(array + num_preds, num_elts - num_preds, size_cutoff);
      int64_t num_true_preds = stridedPartition(array, num_preds, pivot - 1);
	  assert(num_true_preds == get_num_preds(array, num_preds, pivot - 1));
      parallel_spawn parallel_quicksort_strided_partition(array, num_true_preds, size_cutoff);
    } else {
      parallel_spawn parallel_quicksort_strided_partition(array, num_preds, size_cutoff);
      parallel_spawn parallel_quicksort_strided_partition(array + num_preds, num_elts - num_preds, size_cutoff);
    }
    parallel_sync;
  }
}

/*
 * Parallel quicksort implementation using our cache-friendly parallel
 * partition. We swap to serial algorithm once problem sizes get to
 * size n / 10 num-threads or smaller.
 */
void parallel_quicksort_strided_partition (int64_t* array, uint64_t num_elts) {
  int64_t num_threads =  __cilkrts_get_nworkers();
  int64_t size_cutoff = num_elts / (num_threads * 8); 
  parallel_quicksort_strided_partition(array, num_elts, size_cutoff);
}

