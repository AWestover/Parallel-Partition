
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
using namespace std;

// version 1: without blocks  *****************************************************************************
// NOTE: some functions were renamed to avoid naming collisions with functions in cache_friendly_partition.cpp
// this does not mean that they are different functions

struct Vs {
	int64_t vmin; 
	int64_t vmax;
};

/*
 * INPUT: an array A of size n, the size n, and the pivotVal which the array will be partitioned with respect to
 * OUTPUT: the function partitions the array A in place and returns the number of predecessors in the array
 * */
// we partition sufficiently small arrays in serial, because at this point the overhead is too big to make parallelizing it make sense 
int64_t serialPartition_strided(int64_t* A, int64_t n, int64_t pivotVal) {
    int64_t low = 0; int64_t high = n-1;
    while(low < high){
        while(A[low] <= pivotVal && low < high) { // we have to check low < high because otherwise [1 2 3 4 5] with pivot value >= 5 would cause a problem
            low++;
        }
        while (A[high] > pivotVal && low < high){
            high--;
        }
        // swap
        int64_t tmp = A[low];
        A[low] = A[high];
        A[high] = tmp;

		// this is messed up right now...
		// we know already that 
		// A[low] <= pivotVal, A[high] > pivotVal, this saves us 1 check of the while loops above
		// low++; high--;
    }
	// also part of messed up thing
	// low--; // shouldn't have incremented on the last itteration of the while loop, this neutralizes the final low++
    if(low < n && A[low] <= pivotVal){
        low++;
    }
    return low;
}

/*
 * INPUT: 
 * 	startP is the index of the first P_i that we are considering
 * 	numPs is the number of P_i that we are considering
 *	i.e. we are going to calculate min vy and max vy for P_startP, P_startP+1, ..., P_startP+numPs -1
 *
 *	A the array
 *	pivotVal what we are partitioning with respect to
 *	b is the block size
 *	n is the size of A
 *	t is the offset determining the elts in each P_i
 * */
Vs serialStridedPartitions(int64_t startP, int64_t numPs, int64_t *A, int64_t pivotVal, int64_t b, int64_t n, int64_t t){
	if(numPs == 1){
		int64_t low = startP;
		int64_t high = startP + (n/t)*t < n ? startP + (n/t)*t: startP + ((n/t)-1)*t;
		while(low < high){
			while(A[low] <= pivotVal && low < high) { // we have to check low < high because otherwise [1 2 3 4 5] with pivot value 10 would cause a problem
				low+=t;
			}
			while (A[high] > pivotVal && low < high){
				high-=t;
			}
			// swap
			int64_t tmp = A[low];
			A[low] = A[high];
			A[high] = tmp;
		}
		if(low + t <= n && A[low] <= pivotVal){
			low+=t;
		}
		Vs vscur;
		vscur.vmin = low;
		vscur.vmax = low;
		return vscur;
	}
	else{
		Vs vsleft = parallel_spawn serialStridedPartitions(startP, numPs/2, A, pivotVal, b, n, t);
		Vs vsright = serialStridedPartitions(startP + numPs/2, numPs/2+(numPs&1), A, pivotVal, b, n, t);
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

	Vs vsfinal = serialStridedPartitions(0, t, A, pivotVal, b, n, t);
	int64_t num_preds = vsfinal.vmin + serialPartition_strided(A+vsfinal.vmin, vsfinal.vmax - vsfinal.vmin, pivotVal);
	if(num_preds < n && A[num_preds] <= pivotVal){
		num_preds += 1;
	}
	return num_preds;
}

// This is for testing correctness of the partition function when invoked by the sort function. Right now it is not passing!
// TODO: Comment out the use of this in final version.
int64_t get_num_preds_strided(int64_t* array, int64_t n, int64_t pivot) {
  int64_t num_preds = 0;
  for (int i = 0; i < n; i++) {
	if (array[i] <= pivot) num_preds++;
  }
  for (int i = 0; i < num_preds; i++) {
	if (array[i] > pivot) {
	  cout<<n<<" "<<pivot<<endl;
	  for (int j = 0; j < n; j++) cout<<array[j]<<endl;
	}
	assert(array[i] <= pivot);
  }
  return num_preds;
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

	assert(num_preds == get_num_preds_strided(array, num_elts, pivot));
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
	  assert(num_true_preds == get_num_preds_strided(array, num_preds, pivot - 1));
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
