/*
 * Implements variants of parallel partition and serial partition.
 */

#include "params.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <cstdlib>
#include <math.h>
#include <assert.h>
#include <unordered_set>
#include <cstdint>
#include <bits/stdc++.h> 
#include <sys/time.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <string>
#include "libc_partition.h"

using namespace std;


/*
 *  A simple implementation of the serial partitioning
 *  algorithm. Returns number of elts <= pivot.
 */
int64_t serial_partition(int64_t *array, uint64_t n, int64_t pivot) {
  int64_t swap_index = n - 1;
  while (array[swap_index] > pivot) swap_index--;
  for (int64_t i = 0; i <= swap_index; i++) {
    if (array[i] > pivot) {
      int64_t tmp = array[i];
      array[i] = array[swap_index];
      array[swap_index] = tmp;
      swap_index--;
      while (array[swap_index] > pivot) swap_index--;
    }
  }
  return swap_index + 1;
}

/*
 * Replaces array with a prefix sum of itself. Implements parallel
 * prefix sum algorithm. Uses extra memory to be cache friendly, but
 * we only feed arrays of size n/BLOCK_SIZE into this function anyway,
 * so the extra memory is insignificant.
 */
void prefix_sum_unoptimized_cache_friendly(int64_t *array, uint64_t n) {
  if (n <= 1) return;
  int64_t *small_array = (int64_t*)malloc(sizeof(int64_t) * n / 2);
//#pragma grainsize = ((n / 2) / (8 * __cilkrts_get_nworkers()))
  parallel_for (int64_t i = 0; i < n / 2; i++) {
    small_array[i] = array[2 * i] + array[2 * i + 1];
  }
  prefix_sum_unoptimized_cache_friendly(small_array, n / 2);
  array[1] = array[1] + array[0];
//#pragma grainsize = (n / 2) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 1; i < n / 2; i++) {
    array[2 * i] = small_array[i - 1] + array[2 * i];
    array[2 * i + 1] = small_array[i];
  }
  if (n % 2 == 1) {
    array[n - 1] = small_array[n / 2 - 1] + array[n - 1];
  }
  free(small_array);
}


/*
 * First reduces the size of the prefix sum by BLOCK_SIZE, and then
 * outsources it to prefix_sum_unoptimized_cache_friendly.
 */
void prefix_sum_optimized_cache_friendly(int64_t *array, uint64_t n) {
  int64_t *small_array = (int64_t*)malloc(sizeof(int64_t) * n / BLOCK_SIZE);
//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n / BLOCK_SIZE; i++) {
    int64_t sum = 0;
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      sum += array[j];
    }
    small_array[i] = sum;
  }
  prefix_sum_unoptimized_cache_friendly(small_array, n / BLOCK_SIZE);
//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n / BLOCK_SIZE; i++) {
    int64_t prefix_sum = 0;
    if (i > 0) prefix_sum = small_array[i - 1];
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      prefix_sum += array[j];
      array[j] = prefix_sum;
    }
  }
  // This is to handle the case that n is not a multiple of block-size
  int64_t prefix_sum = small_array[n / BLOCK_SIZE - 1];
  for (int64_t j = (n / BLOCK_SIZE) * BLOCK_SIZE; j < n; j++) {
    prefix_sum += array[j];
    array[j] = prefix_sum;
  }
  free(small_array);
}

/*
 * Replaces array with a prefix sum of itself. Implements parallel
 * prefix sum algorithm.
 */
void prefix_sum_unoptimized(int64_t *array, uint64_t n) {
  prefix_sum_unoptimized_cache_friendly(array, n);
  return;
  
  // The below code also works and does the prefix sum in-place,
  // though for ease of implementation I assume n is a power of
  // two. The function call above runs slightly faster, however.
  
  assert((n & (n - 1)) == 0); // Assert n is a pwer of two
  int64_t jump;
  for (jump = 2; jump <= n; jump *= 2) {
//#pragma grainsize = (n / jump) / (8 * __cilkrts_get_nworkers())
    parallel_for (int64_t i = n; i >= jump; i -= jump) {
      array[i - 1] += array[i - jump / 2 - 1];
    }
  }
  for (; jump >= 2; jump /= 2) {
//#pragma grainsize = (n / jump) / (8 * __cilkrts_get_nworkers()) 
    parallel_for (int64_t i = jump / 2 + jump; i <= n; i+= jump) {
      array[i - 1] = array[i - jump / 2 - 1] + array[i - 1];
    }
  }
}

/*
 * Replaces array with a prefix sum of itself. Implements parallel
 * prefix sum, and begins by reducing to a problem of size BLOCK_SIZE
 * smaller before reverting to standard parallel prefix algorithm.
 */
void prefix_sum_optimized(int64_t *array, uint64_t n) {
  prefix_sum_optimized_cache_friendly(array, n);
  return;

  // The below code also works and does the prefix sum in-place,
  // though for ease of implementation I assume n is a power of
  // two. The function call above runs slightly faster, however.

  assert((n & (n - 1)) == 0); // Assert n is a pwer of two
  if (n < BLOCK_SIZE) {
    prefix_sum_unoptimized(array, n);
    return;
  }
  int64_t jump;
//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n / BLOCK_SIZE; i++) {
    int64_t sum = 0;
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      sum += array[j];
      array[j] = sum;
    }
  }
  for (jump = BLOCK_SIZE * 2; jump <= n; jump *= 2) {
//#pragma grainsize = (n / jump) / (8 * __cilkrts_get_nworkers())
    parallel_for (int64_t i = n; i >= jump; i -= jump) {
      array[i - 1] += array[i - jump / 2 - 1];
    }
  }
  for (; jump >= BLOCK_SIZE * 2; jump /= 2) {
//#pragma grainsize = (n / jump) / (8 * __cilkrts_get_nworkers())    
    parallel_for (int64_t i = jump / 2 + jump; i <= n; i+= jump) {
      array[i - 1] = array[i - jump / 2 - 1] + array[i - 1];
    }
  }
//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 1; i < n / BLOCK_SIZE; i++) {
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE - 1; j++) {
      array[j] += array[i * BLOCK_SIZE - 1];
    }
  }
}



/*
 * A simple test for prefix_sum.
 */
void test_prefix_sum() {
  if (BLOCK_SIZE >= 1411) return; // For this case, the folowing test doesn't work
  int64_t array[1411];
  for (int64_t i = 0; i < 1411; i++) array[i] = 1;
  prefix_sum_optimized(array, 1411);
  for (int64_t i = 0; i < 1411; i++) assert(array[i] == i + 1);

  int64_t array2[1411];
  for (int64_t i = 0; i < 1411; i++) array2[i] = 1;
  prefix_sum_optimized(array2, 1411);
  for (int64_t i = 0; i < 1411; i++) assert(array2[i] == i + 1);
}

/*
 * Partitions array so that elements <= pivot appear before other
 * elts. Uses standard algorithm.
 */
void partition(int64_t *array, uint64_t n, int64_t pivot) {
  int64_t *prefixes = (int64_t*) malloc(sizeof(int64_t) * n);
//#pragma grainsize = (n) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n; i++) {
    prefixes[i] = (array[i] <= pivot);
  }
  prefix_sum_optimized(prefixes, n);
  int64_t *output_tmp = (int64_t*) malloc(sizeof(int64_t) * n);
  int64_t num_preds = prefixes[n - 1];
//#pragma grainsize = (n) / (8 * __cilkrts_get_nworkers())  
  parallel_for (int64_t i = 0; i < n; i++) {
    if (array[i] <= pivot) {
      output_tmp[prefixes[i] - 1] = array[i];
    } else {
      output_tmp[num_preds + (i - prefixes[i])] = array[i]; 
    }
  }
//#pragma grainsize = (n) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n; i++) {
    array[i] = output_tmp[i];
  }
  free(output_tmp);
  free(prefixes);
}

/*
 * Performs the reordering step of the partitioning algorithm
 * in-place; requires the preprocessing step that ensures all prefixes
 * are successor-heavy. Assumes that only every BLOCK_SIZE-th prefixes
 * entry is actually stored.
 */
void in_place_reorder(int64_t* array, int64_t* prefixes, uint64_t n, int64_t pivot) {
  if (n <= BLOCK_SIZE * 10) {
    // We just take the easy way out here
    sort(array, array + n);
    return;
  }
  // m is the size on which we recurse. The constant in the paper is
  // easily improved to 3/4 rather than 4/5. The bit-hack at the end
  // just makes m a multiple of BLOCK_SIZE, which is convenient.
  int64_t m = (n * 3 / 4 + BLOCK_SIZE) & (~(BLOCK_SIZE - 1));
  in_place_reorder(array, prefixes, m, pivot);

//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t j = m / BLOCK_SIZE; j < n / BLOCK_SIZE; j++) {
    int64_t new_pos = prefixes[j - 1];
    for (int64_t i = j * BLOCK_SIZE; i < j * BLOCK_SIZE + BLOCK_SIZE; i++) {
      if (array[i] <= pivot) {
        int64_t tmp = array[i];
        array[i] = array[new_pos];
        array[new_pos] = tmp;
        new_pos++; 
      }
    }
  }
  // Now we handle case that n is not a multiple of BLOCK_SIZE, though
  // this will only happen once at the top level of recursion.
  int64_t j = n / BLOCK_SIZE;
  int64_t new_pos = prefixes[j - 1];
  for (int64_t i = j * BLOCK_SIZE; i < n; i++) {
    if (array[i] <= pivot) {
      int64_t tmp = array[i];
      array[i] = array[new_pos];
      array[new_pos] = tmp;
      new_pos++; 
    }
  }    
}

/*
 * Performs the reordering step of the partitioning algorithm
 * in-place; requires the preprocessing step that ensures all suffixes
 * are predecessor-heavy. Assumes that only every BLOCK_SIZE-th
 * prefixes entry is actually stored. Unlike in in_place_reorder,
 * prefixes[i] is the number of successors in the first i blocks,
 * rather than the number of predecessors. To help with bookkeeping,
 * we also have an extra argument num_all_successors denoting the
 * number of successors at the top level of recursion.
 */
void backward_in_place_reorder(int64_t* array, int64_t* prefixes, uint64_t n, int64_t pivot, int64_t num_all_successors) {
  if (n <= BLOCK_SIZE * 10) {
    // We just take the easy way out here
    sort(array, array + n);
    return;
  }
  
  int64_t subproblem_start = (n / 4) & (~((uint64_t)BLOCK_SIZE - 1));
  int64_t m = n - subproblem_start;
  // we recurse on the final m elements of the array
  backward_in_place_reorder(array + subproblem_start, prefixes + subproblem_start / BLOCK_SIZE, m, pivot, num_all_successors);
  int64_t jmax = (n - m - 1) / BLOCK_SIZE;
  // This is the position where the very first successor would go:
  int64_t offset = n - num_all_successors;

  // We loop through the blocks in parallel:
//#pragma grainsize = (jmax) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t j = 0;  j < jmax; j++) {
    // Within each block we perform the appropriate swaps
    // new_pos is the position that the first successor in the block will belong in
    int64_t new_pos = offset + prefixes[j - 1];
    for (int64_t i = j * BLOCK_SIZE; i < j * BLOCK_SIZE + BLOCK_SIZE; i++) {
      if (array[i] > pivot) {
        int64_t tmp = array[i];
        array[i] = array[new_pos];
        array[new_pos] = tmp;
        new_pos++;
      } 
    }
  }

  // Now we handle case that n is not a multiple of BLOCK_SIZE.
  int64_t j = (n - m - 1) / BLOCK_SIZE;
  int64_t new_pos =  offset + prefixes[j - 1];
  for (int64_t i = j * BLOCK_SIZE; i < n - m; i++) {
    if (array[i] > pivot) {
      int64_t tmp = array[i];
      array[i] = array[new_pos];
      array[new_pos] = tmp;
      new_pos++;
    }
  }    
}


/*
 * Partitions array so that elements <= pivot appear before other
 * elts. Uses algorithmic techniques to reduce memory footprint
 * for prefix sums
 */
void small_prefix_partition(int64_t *array, uint64_t n, int64_t pivot) {
  int64_t *prefixes = (int64_t*) malloc(sizeof(int64_t) * n / BLOCK_SIZE);
//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n / BLOCK_SIZE; i++) {
    int64_t sum = 0;
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      sum += (array[j] <= pivot);
    }
    prefixes[i] = sum;
  }
  // By using the unoptimized prefix sum (without the initial
  // reduction to a smaller prefix sum), we make it so the advantage
  // we're getting really is from our use of less memory.
  prefix_sum_unoptimized(prefixes, n / BLOCK_SIZE);
  int64_t *output_tmp = (int64_t*) malloc(sizeof(int64_t) * n);
  int64_t num_preds = prefixes[n / BLOCK_SIZE - 1];
  for (int64_t i = (n / BLOCK_SIZE) * BLOCK_SIZE; i < n; i++) num_preds += (array[i] <= pivot);
//#pragma grainsize = (n / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n / BLOCK_SIZE; i++) {
    int64_t prefix_sum = 0;
    if (i > 0) prefix_sum = prefixes[i - 1];
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      if (array[j] <= pivot) {
        prefix_sum++;
        output_tmp[prefix_sum - 1] = array[j];
      } else {
        output_tmp[num_preds + (j - prefix_sum)] = array[j]; 
      }
    }
  }

  
  // Now we have to handle case where n is not a multiple of BLOCK_SIZE
  if (true) { // This is just so that I can make i a local variable
    int64_t i = n / BLOCK_SIZE;
    int64_t prefix_sum = 0;
    prefix_sum = prefixes[i - 1];
    for (int64_t j = i * BLOCK_SIZE; j < n; j++) {
      if (array[j] <= pivot) {
        prefix_sum++;
        output_tmp[prefix_sum - 1] = array[j];
      } else {
        output_tmp[num_preds + (j - prefix_sum)] = array[j]; 
      }
    }
  }
  
//#pragma grainsize = (n) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n; i++) {
    array[i] = output_tmp[i];
  }
  free(output_tmp);
  free(prefixes);
}

/*
 * This completes in_place_partition in the case where the number of
 * successors is round to be smaller than the number of predecessors.
 */
void finish_backward_in_place_partition(int64_t *array, uint64_t n, int64_t pivot, int64_t num_successors) {

  // For convenience, we define prefixes to be an array such that prefixes[-1] = 0;
  int64_t *prefixes_shifted = (int64_t*) malloc(sizeof(int64_t) * (n / BLOCK_SIZE + 1));
  prefixes_shifted[0] = 0;
  int64_t* prefixes = prefixes_shifted + 1;

  // Now we do the rest of the preprocessing step. At the same time,
  // we fill in the values of prefixes for the right half of the
  // array. 
  int64_t *backward_array = array + n - 1;
  for (int64_t m = (n + 1) / 2; m > 1; m = (m + 1) / 2) {
    // The code is similar as in in_place_partition, except now we're
    // indexing from the back of the array rather than the front

    // In order to fill in entries of prefixes, we need to be
    // considering things in aligned BLOCK_SIZE-sized chunks. We
    // define shift to be the offset mod BLOCK_SIZE that we need to
    // start at in order for this to work right. We need shift to have
    // the property that n - (m + 1) / 2) - shift == 0 mod BLOCK_SIZE.
    int64_t shift = (n - 1 - (m + 1) / 2) % BLOCK_SIZE;

    // The first shift + 1 elements of the preprocessing we do without
    // any attention being given to prefixes
    for (int64_t i = 0; i <= shift; i++) {
      if (backward_array[-i] > pivot) {
        int64_t tmp = backward_array[-i];
        backward_array[-i] = backward_array[-i - (m + 1) / 2];
        backward_array[-i - (m + 1) / 2] = tmp;
      }
    }
    // Now we do the main part of the loop:
//#pragma grainsize = (m / (2 * BLOCK_SIZE)) / (8 * __cilkrts_get_nworkers())
    parallel_for (int64_t j = shift; j < m / 2; j += BLOCK_SIZE) {
      int64_t sum = 0;
      // Now we iterate through a block of size BLOCK_SIZE
      for (int64_t i = j + 1; i <= j + BLOCK_SIZE; i++) {
        if (backward_array[-i] > pivot && i < m / 2) {
          int64_t tmp = backward_array[-i];
          backward_array[-i] = backward_array[-i - (m + 1) / 2];
          backward_array[-i - (m + 1) / 2] = tmp;
        }
        sum += (backward_array[-i - (m + 1) / 2] > pivot);
      }
      // Fill in the appropriate entry of prefixes:
      prefixes[(n - 1 - (m + 1) / 2 - j) / BLOCK_SIZE - 1] = sum;
    }
  }
  
    
  // Now the prefix sum step. The second half of the prefixes array is
  // already filled in, which lets us save a lot of potential cache
  // misses here.
//#pragma grainsize = (n / (2 * BLOCK_SIZE)) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = 0; i < n / (2 * BLOCK_SIZE); i++) {
    int64_t sum = 0;
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      sum += (array[j] > pivot);
    }
    prefixes[i] = sum;
  }
  // To handle an edge case:
  for (int64_t i = n / BLOCK_SIZE - 1; i < n / BLOCK_SIZE; i++) {
    int64_t sum = 0;
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      sum += (array[j] > pivot);
    }
    prefixes[i] = sum;
  }

  // By using the unoptimized prefix sum, we make it so the advantage
  // we're getting really is from our use of less memory, and so that
  // our span is similar to that of the medium-memory algorithm.
  prefix_sum_unoptimized_cache_friendly(prefixes, n / BLOCK_SIZE);
  // Now the reordering step:
  backward_in_place_reorder(array, prefixes, n, pivot, num_successors);
  free(prefixes_shifted);
}


/*
 *  Counts number of predecessors in array. Also performs first run of
 *  preprocessing phase for in-place algorithm.
 */ 
int64_t preprocessing_run(int64_t *array, uint64_t bottom, uint64_t top, uint64_t n, int64_t pivot) {
  // To offset cost of cilk spawns, we convert to a serial for loop at
  // the same granularity as CILK uses by default in for-loops.
  if (top - bottom <= 4096) {
    int64_t num_pred = 0;
    for (int64_t i = bottom; i < top; i++) {
      int64_t pos2 = (n + 1) / 2 + i;
      num_pred += (array[pos2] <= pivot);
      if (array[i] <= pivot) {
        num_pred++;
        int64_t tmp = array[i];
        array[i] = array[pos2];
        array[pos2] = tmp;
      }
    }
    return num_pred;
  } else {
    int64_t middle = ((bottom + top) / 2);
    int64_t a1 = parallel_spawn preprocessing_run(array, bottom, middle, n, pivot);
    int64_t a2 = parallel_spawn preprocessing_run(array, middle, top, n, pivot);
    parallel_sync;
    return a1 + a2;
  }
}



/*
 * Partitions array so that elements <= pivot appear before other
 * elts. Uses algorithmic techniques to reduce memory footprint
 * (though not all the way in place). Returns number of elts <= pivot.
 */
uint64_t in_place_partition(int64_t *array, uint64_t n, int64_t pivot) {
  if (n <= BLOCK_SIZE * 2) return serial_partition(array, n, pivot);
  // During the first pass of the preprocessing phase, we also count
  // the number of predecessors, since whether this is <= n / 2
  // determines which of the two symmetric versions of the algorithm
  // we run. We combine the first run of the preprocessing phase with
  // the counting of the number of predecessors; to make the counting
  // parallelized, we implement the for-loop recursively. (Using CILK
  // reducers, which is the standard alternative performs much worse.)

  int64_t num_pred = preprocessing_run(array, 0, n / 2, n, pivot);
  if (n % 2 == 1) num_pred += (array[n / 2] <= pivot);
  if (num_pred > n / 2) {
    finish_backward_in_place_partition(array, n, pivot, n - num_pred);
    return num_pred;
  }
  int64_t *prefixes = (int64_t*) malloc(sizeof(int64_t) * n / BLOCK_SIZE);

  // Now the rest of the preprocessing step.  At the same time, we
  // fill in the values of prefixes for the left half of the array.
  for (int64_t m = (n + 1) / 2; m > 1; m = (m + 1) / 2) {
    // In order to fill in entries of prefixes, we need to be
    // considering things in aligned BLOCK_SIZE-sized chunks. We
    // define shift to be the offset mod BLOCK_SIZE that we need to
    // start at in order for this to work right. In particular, we
    // define it to be the amount thatneeds to be added to (m + 1) / 2
    // to get to a multiple of BLOCK_SIZE.

    int64_t shift = (((m + 1) / 2 + BLOCK_SIZE - 1) & (~(BLOCK_SIZE - 1))) - (m + 1) / 2;
    // For the first shift elements of the preprocessing, we don't do
    // anything to the prefixes array.
    for (int64_t i = 0; i < shift; i++) {
      if (array[i] <= pivot) {
        int64_t tmp = array[i];
        array[i] = array[(m + 1) / 2 + i];
        array[(m + 1) / 2 + i] = tmp;
      }
    }
    // Now we do the main part of the loop:
//#pragma grainsize = ((m - shift) / BLOCK_SIZE) / (8 * __cilkrts_get_nworkers())
    parallel_for (int64_t j = shift; j < m / 2; j += BLOCK_SIZE) {
      int64_t sum = 0;
      // Now we iterate through a block of size BLOCK_SIZE
      for (int64_t i = j; i < j + BLOCK_SIZE; i++) {
        if (array[i] <= pivot && i < m / 2) {
          int64_t tmp = array[i];
          array[i] = array[(m + 1) / 2 + i];
          array[(m + 1) / 2 + i] = tmp;
        }
        sum += (array[(m + 1) / 2 + i] <= pivot);
      }
      // Fill in the appropriate entry of prefixes:
      prefixes[((m + 1) / 2 + j) / BLOCK_SIZE] = sum;
    }
  }

  // We handle the edge case of prefixes[0] not being filled in correctly
  prefixes[0] = 0;
  for (int64_t j = 0; j < BLOCK_SIZE; j++) {
    prefixes[0] += (array[j] <= pivot);
  }


  // Now the prefix sum step. The first half of th eprefixes array is
  // already filled in, which lets us save a lot of potential cache
  // misses here.
//#pragma grainsize = (n / (2 * BLOCK_SIZE)) / (8 * __cilkrts_get_nworkers())
  parallel_for (int64_t i = n / (2 * BLOCK_SIZE); i < n / BLOCK_SIZE; i++) {
    int64_t sum = 0;
    for (int64_t j = i * BLOCK_SIZE; j < i * BLOCK_SIZE + BLOCK_SIZE; j++) {
      sum += (array[j] <= pivot);
    }
    prefixes[i] = sum;
  }

  // By using the unoptimized prefix sum, we make it so the advantage
  // we're getting really is from our use of less memory, and so that
  // our span is similar to that of the medium-memory algorithm.
  prefix_sum_unoptimized(prefixes, n / BLOCK_SIZE);
  // Now the reordering step:
  in_place_reorder(array, prefixes, n, pivot);
  free(prefixes);
  return num_pred;
}


/*
 * Parallel quicksort implementation using our low-space parallel
 * partition. This is called internally by the parallel_quicksort with
 * fewer arguments. Once problem-sizes get to size_cutoff or smaller,
 * we swap to serial.
 */
void parallel_quicksort(int64_t* array, uint64_t num_elts, uint64_t size_cutoff) {
  if (num_elts <= 1) return;
  if (num_elts <= size_cutoff) {
    libc_quicksort(array, num_elts);
  } else {
    int64_t pivot = select_pivot(array, num_elts);
    int64_t num_preds = in_place_partition(array, num_elts, pivot);
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
      parallel_spawn parallel_quicksort(array + num_preds, num_elts - num_preds, size_cutoff);
      int64_t num_true_preds = in_place_partition(array, num_preds, pivot - 1);
      parallel_spawn parallel_quicksort(array, num_true_preds, size_cutoff);
    } else {
      parallel_spawn parallel_quicksort(array, num_preds, size_cutoff);
      parallel_spawn parallel_quicksort(array + num_preds, num_elts - num_preds, size_cutoff);
    }
    parallel_sync;
  }
}

/*
 * Parallel quicksort implementation using our low-space parallel
 * partition. We swap to serial algorithm once problem sizes get to
 * size n / 10 num-threads or smaller.
 */
void parallel_quicksort (int64_t* array, uint64_t num_elts) {
  int64_t num_threads = __cilkrts_get_nworkers();
  int64_t size_cutoff = num_elts / (num_threads * 8); 
  parallel_quicksort(array, num_elts, size_cutoff);
}



/* 
 * Inputs an array, two positions pos_new < pos_old, and a
 * size. Rearranges array so that the size elements after pos_new are
 * now after pos_old (but not necessarily in the same order as before).
 */
void transfer_back(int64_t* array, uint64_t pos_old, uint64_t pos_new, uint64_t size) {
  if (pos_new + size > pos_old) {
    uint64_t pos_old2 = pos_new + size;
    uint64_t size2 = pos_old + size - pos_old2;
    pos_old = pos_old2;
    size = size2;
  }
  parallel_for (uint64_t i = 0; i < size; i++) {
    uint64_t tmp = array[pos_new + i];
    array[pos_new + i] = array[pos_old + i];
    array[pos_old + i] = tmp;
  }
}

/*
 * Inputs an array that has been partitioned based on <= pivot or >
 * pivot. Performs a binary search and returns the size of the first
 * part of the partition.
 */
uint64_t find_split(int64_t* array, uint64_t size, int64_t pivot) {
  if (size == 0) return 0;
  if (array[0] > pivot) return 0;
  int64_t left = 0;
  int64_t right = size;
  while (left  < right - 1) {
    int64_t middle = (left + right) / 2;
    if (array[middle] <= pivot) left = middle;
    else right = middle;
  }
  return left + 1;
}

/*
 * This is a simple in-place algorithm that has poor theoretical
 * guarantees on its span; but when tuned appropriately can be made to
 * work very well.
 */
uint64_t high_span_partition(int64_t* array, uint64_t n, int64_t pivot) {
  uint64_t chunk_size =  n / max(BLOCK_SIZE, (8 * __cilkrts_get_nworkers()));
  if (chunk_size == 0) return serial_partition(array, n, pivot);
  uint64_t num_chunks = n / chunk_size + 1;
  parallel_for(uint64_t chunk = 0; chunk < num_chunks; chunk++) {
    uint64_t chunk_start = chunk * chunk_size;
    int64_t shortened_chunk_size = min((chunk + 1) * chunk_size, n) - chunk_start;
    libc_partition_strict(array + chunk_start, shortened_chunk_size, pivot);
  }
  uint64_t num_preds_total = find_split(array, chunk_size, pivot);
  for (uint64_t chunk = 1; chunk < num_chunks; chunk++) {
    uint64_t chunk_start = chunk * chunk_size;
    int64_t shortened_chunk_size = min((chunk + 1) * chunk_size, n) - chunk_start;
    uint64_t num_preds = find_split(array + chunk_start, shortened_chunk_size, pivot);
    transfer_back(array, chunk_start, num_preds_total, num_preds);
    num_preds_total += num_preds;
  }
  return num_preds_total;
}

/*
 * Parallel quicksort implementation using our high-space parallel
 * partition. This is called internally by the parallel_quicksort with
 * fewer arguments. Once problem-sizes get to size_cutoff or smaller,
 * we swap to serial.
 */
void parallel_quicksort_high_span(int64_t* array, uint64_t num_elts, uint64_t size_cutoff) {
  if (num_elts <= 1) return;
  if (num_elts <= size_cutoff) {
    libc_quicksort(array, num_elts);
  } else {
    int64_t pivot = select_pivot(array, num_elts);
    int64_t num_preds = high_span_partition(array, num_elts, pivot);
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
      parallel_spawn parallel_quicksort_high_span(array + num_preds, num_elts - num_preds, size_cutoff);
      int64_t num_true_preds = high_span_partition(array, num_preds, pivot - 1);
      parallel_spawn parallel_quicksort_high_span(array, num_true_preds, size_cutoff);
    } else {
      parallel_spawn parallel_quicksort_high_span(array, num_preds, size_cutoff);
      parallel_spawn parallel_quicksort_high_span(array + num_preds, num_elts - num_preds, size_cutoff);
    }
    parallel_sync;
  }
}

/*
 * Parallel quicksort implementation using our high-span  parallel
 * partition. We swap to serial algorithm once problem sizes get to
 * size n / 10 num-threads or smaller.
 */
void parallel_quicksort_high_span (int64_t* array, uint64_t num_elts) {
  int64_t num_threads =  __cilkrts_get_nworkers();
  int64_t size_cutoff = num_elts / (num_threads * 8); 
  parallel_quicksort_high_span(array, num_elts, size_cutoff);
}

