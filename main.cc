/*
 *
 * Runs timing (and correctness) tests on the partition functions in partition.cc/h
 */

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
#include "partition.h"
#include "libc_partition.h"
#include "cache_friendly_partition.h"


using namespace std;

struct timeval tp;



void set_num_threads(int64_t num_threads) {
  __cilkrts_end_cilk();
  string str1 = "nworkers";
  string str2 = to_string(num_threads);
  assert(__cilkrts_set_param((char*)str1.c_str(), (char*)str2.c_str()) == 0);
  assert(__cilkrts_get_nworkers() == num_threads);
}


/*
 * A simple test for variants of partition. The argument in_place
 * determines which version of the algorithm we run. Returns number of
 * milliseconds that test took. Depending on the value of more_succ,
 * we either guarantee that the input has more predecessors than
 * successors or more successors than predecessors. For the in-place
 * algorithm this results in two different cases; our implementation
 * for the case where more_succ == false is slightly slower, so we
 * use it for all the experiments.
 *
 */
int64_t test_partition(int64_t in_place, uint64_t n, bool more_succ, uint64_t num_threads) {
  // Interesting Remark: Frusteratingly, restarting/resetting the
  // threads right before the partition vs in other places seems to
  // sometimes actually affect the performance of the partition on
  // some machines. This seems to be some strange behavior of cilk. To
  // make it so that all the experiments are in the same setting, I
  // always set the number of threads right before the computation at
  // hand.
  set_num_threads(num_threads);
  int64_t answer;
  int64_t *array = (int64_t*) malloc(sizeof(int64_t) * n);
  uint64_t num_zeros;
  do {
    num_zeros = 0;
    // We fill the array with zeros and 100s.
    for (int64_t i = 0; i < n; i++) {
      array[i] = rand() % 2 * 100;
      if (array[i] == 0) num_zeros++;
    }
  } while ((((num_zeros > n / 2) && more_succ) || ((num_zeros < n / 2) && !more_succ)));
  if (in_place == -3) { 
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    // Our implementation. Better to use libc's though.
    serial_partition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }
  if (in_place == -2) { 
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    // Our implementation. Better to use libc's though.
    serial_partition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }
  if (in_place == -1) { 
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    libc_partition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }
  if (in_place == 0) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    in_place_partition(array, n, 50); // We use pivot 50
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }
  if (in_place == 1) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    small_prefix_partition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = ms2 - ms1;
  }
  if (in_place == 2) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    partition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = ms2 - ms1;
  }
  if (in_place == 3) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    high_span_partition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = ms2 - ms1;
  }
  if(in_place == 4) {
	gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    groupedPartition(array, n, 50);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = ms2 - ms1;
  }
  // We verify correctness of output:
  for (int64_t i = 0; i < num_zeros; i++) {
    assert(array[i] == 0);
  }
  for (int64_t i = num_zeros; i < n; i++) {
    assert(array[i] == 100);
  }
  free(array);
  return answer;
}


/*
 * Averages answer of test_partition over multiple trials.
 */
double test_partition_multiple_trials(int64_t in_place, uint64_t n, uint64_t num_trials, bool more_succ, uint64_t num_threads) {
  int64_t answer = 0;
  for (int64_t i = 0; i < num_trials; i++) {
    answer += test_partition(in_place, n, more_succ, num_threads);
  }
  if (answer == 0) answer++;
  //cout<<endl<<(double)answer / (double)num_trials<<" ms"<<endl;
  return (double)answer / (double)num_trials;
}

/*
 * A simple test for comparing serial and parallel sort. The argument
 * type determines which version of the algorithm we run. Returns
 * number of milliseconds that test took.
 *
 */
int64_t test_sort(int64_t type, int64_t n, uint64_t num_threads) {
  int64_t answer;
  int64_t *array = (int64_t*) malloc((int64_t)sizeof(int64_t) * n);
  for (int64_t i = 0; i < n; i++) {
    array[i] = rand();
  }
  if (type == 0) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    libc_quicksort(array, n);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }
  if (type == 1) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    parallel_quicksort(array, n);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }
  if (type == 2) {
    gettimeofday(&tp, NULL);
    long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    parallel_quicksort_high_span(array, n);
    gettimeofday(&tp, NULL);
    long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    answer = (ms2 - ms1);
  }

  for (int64_t i = 0; i < n - 1; i++) assert(array[i] <= array[i + 1]);
  free(array);
  return answer;  
}

/*
 * Averages answer of test_sort over multiple trials.
 */
double test_sort_multiple_trials(int64_t type, uint64_t n, uint64_t num_trials, uint64_t num_threads) {
  int64_t answer = 0;
  for (int64_t i = 0; i < num_trials; i++) {
    answer += test_sort(type, n, num_threads);
  }
  if (answer == 0) answer++;
  return (double)answer / (double)num_trials;
}


/* 
 *  Performs the parallel partition tests for the paper.
 */ 
void parallel_partition_tests() {
  string tablename = "CILK";
  double serial_baseline = test_partition_multiple_trials(-1, MAX_INPUT_SIZE, NUM_TRIALS, false, 1);
  cout<<"\\def \\"<<tablename<<"serialbaseline {"<<serial_baseline<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"blocksize {"<<BLOCK_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"numtrials {"<<NUM_TRIALS<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"inputsize {"<<MAX_INPUT_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"table {"<<endl;
  cout<<"\\begin{tikzpicture}[scale = .8]"
      <<endl<<"\\begin{axis}["
      <<endl<<"width = 5 in,"
      <<endl<<"height = 4in,"
      <<endl<<"title={Speedup Versus Number of Threads},"
      <<endl<<"xtick pos=left,"
      <<endl<<"ytick pos=left,"
      <<endl<<"legend style={draw=none},"
      <<endl<<"axis line style = { draw = none },"
      <<endl<<"legend pos= north west,"
      <<endl<<"xtick = data,"
      <<endl<<"xlabel={Number of Threads},"
      <<endl<<"ylabel={Speedup Over Serial Partition},"
      <<endl<<"ymax = 8,"
      <<endl<<"legend columns = 2,"
      <<endl<<"scatter/classes=%"
      <<endl<<"{a={mark=o,draw=blue}}]"<<endl;
  
  
  for (int64_t i = 0; i <= 4; i++) {
    if (i == 0) cout<<"%% In-Place"<<endl;
    if (i == 1) cout<<"%% In-Place Prefix-Sum"<<endl;
    if (i == 2) cout<<"%% Out-of-Place"<<endl;
    if (i == 3) cout<<"%% High-Span"<<endl;
	if (i == 4) cout<<"%% Cache-Friendly"<<endl;
    cout<<"\\addplot coordinates {";
    for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
      double millisecs = test_partition_multiple_trials(i, MAX_INPUT_SIZE, NUM_TRIALS, false, num_cores);
      double speedup = serial_baseline / millisecs;
      cout << "( " << num_cores  << ", " << speedup << ") ";
    }
    cout<<"};"<<endl;
  }
  cout<<"\\legend{Low-Space, Med-Space, High-Space, Two-Layer, Cache-friendly}"<<endl;
  cout<<"\\end{axis}"<<endl<<"\\end{tikzpicture}"<<endl<<"}"<<endl;
}


/* 
 *  These are the more parallel partition tests for the paper.
 */ 
void parallel_partition_tests_two() {
  string tablename = "cilktwo";
  cout<<"\\def \\"<<tablename<<"blocksizetwo {"<<BLOCK_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"numtrialstwo {"<<NUM_TRIALS<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"numcorestwo {"<<NUM_THREADS_DEFAULT<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"inputsizetwo {"<<MAX_INPUT_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"tabletwo {"<<endl;
  cout<<"\\begin{tikzpicture}[scale = .8]"
      <<endl<<"\\begin{axis}["
      <<endl<<"width = 5 in,"
      <<endl<<"height = 4in,"
      <<endl<<"title={Speedup Versus Input Size},"
      <<endl<<"xtick pos=left,"
      <<endl<<"ytick pos=left,"
      <<endl<<"legend style={draw=none},"
      <<endl<<"axis line style = { draw = none },"
      <<endl<<"legend pos= north west,"
      <<endl<<"xtick = data,"
      <<endl<<"xlabel={Log Input Size},"
      <<endl<<"ylabel={Speedup Over Serial Partition},"
      <<endl<<"ymax = 5,"
      <<endl<<"ymin = 0,"
      <<endl<<"legend columns = 2,"
      <<endl<<"scatter/classes=%"
      <<endl<<"{a={mark=o,draw=blue}}]"<<endl;
  vector<double> baselines(100, 0);
  for (int64_t i = -1; i <= 4; i+=1) {
    if (i == -1) cout<<"%% Serial Baseline"<<endl<<"%% baselines in ms: ";
    if (i == 0) cout<<"%% In-Place"<<endl;
    if (i == 1) cout<<"%% In-Place Prefix-Sum"<<endl;
    if (i == 2) cout<<"%% Out-of-Place"<<endl;
    if (i == 3) cout<<"%% High-Span"<<endl;
	if (i == 4) cout<<"%% Cache-Friendly"<<endl;
    cout<<"\\addplot coordinates {";
    for (int64_t input_size_log = 23; ((uint64_t)1 << input_size_log) <= MAX_INPUT_SIZE; input_size_log++) {
      int64_t input_size = (1 << input_size_log);
      double millisecs = test_partition_multiple_trials(i, input_size, NUM_TRIALS, false, NUM_THREADS_DEFAULT);
      if (i == -1) {
        baselines[input_size_log] = millisecs;
        assert (input_size_log < 100); // Because I was lazy in allocating space to baselines
        cout << "( " << input_size_log  << ", " << millisecs << " ) ";
      } else {
        double speedup = baselines[input_size_log] / millisecs;
        cout << "( " << input_size_log  << ", " << speedup << ") ";
      }
    }
    cout<<"};"<<endl;
  }
  cout<<"\\legend{Low-Space, Med-Space, High-Space, Two-Layer, Cache-Friendly}"<<endl;
  cout<<"\\end{axis}"<<endl<<"\\end{tikzpicture}"<<endl<<"}"<<endl;
}


/*
 * Performs the serial partition tests for the paper.
 */
void serial_partition_tests() {
  cout<<"\\def \\serialnumtrials {"<<NUM_TRIALS<<"}"<<endl;
  cout<<"\\def \\serialblocksize {"<<BLOCK_SIZE<<"}"<<endl;
  cout<<"\\def \\serialtable {"<<endl;
  cout<<"\\begin{tikzpicture}[scale = .8]"
      <<endl<<"\\begin{axis}["
      <<endl<<"width = 5 in,"
      <<endl<<"height = 4in,"
      <<endl<<"title={Slowdown Versus Input Size in Serial},"
      <<endl<<"xtick pos=left,"
      <<endl<<"ytick pos=left,"
      <<endl<<"ymax = 5,"
      <<endl<<"ymin = 0,"
      <<endl<<"legend style={draw=none},"
      <<endl<<"axis line style = { draw = none },"
      <<endl<<"legend pos= north west,"
      <<endl<<"xtick = data,"
      <<endl<<"xlabel={Log Input Size},"
      <<endl<<"ylabel={Slowdown Over Serial Partition},"
      <<endl<<"legend columns = 2,"
      <<endl<<"scatter/classes=%"
      <<endl<<"{a={mark=o,draw=blue}}]"<<endl;

  vector<double> baselines(100, 0);
  for (int64_t i = -1; i <= 4; i++) {
    if (i == -1) cout<<"%% Serial Baseline"<<endl<<"%% baselines in ms: ";
    if (i == 0) cout<<"%% In-Place"<<endl;
    if (i == 1) cout<<"%% In-Place Prefix-Sum"<<endl;
    if (i == 2) cout<<"%% Out-of-Place"<<endl;
    if (i == 3) cout<<"%% High-Span"<<endl;
	if (i == 4) cout<<"%% Cache-Friendly"<<endl;
    cout<<"\\addplot coordinates {";
    for (int64_t input_size_log = 23; ((uint64_t)1 << input_size_log) <= MAX_INPUT_SIZE; input_size_log++) {
      int64_t input_size = (1 << input_size_log);
      double millisecs = test_partition_multiple_trials(i, input_size, NUM_TRIALS, false, 1);
      if (i == -1) {
        baselines[input_size_log] = millisecs;
        assert (input_size_log < 100); // Because I was lazy in allocating space to baselines
        cout << "( " << input_size_log  << ", " << millisecs << " ) ";
      } else {
        double slowdown = millisecs / baselines[input_size_log];
        cout << "( " << input_size_log  << ", " << slowdown << ") ";
      }
    }
    cout<<"};"<<endl;
  }
  cout<<"\\legend{Low-Space, Med-Space, High-Space, Two-Layer, Cache-Friendly}"<<endl;
  cout<<"\\end{axis}"<<endl<<"\\end{tikzpicture}"<<endl<<"}"<<endl;
}

/* 
 *  Performs the parallel sort tests for the paper.
 */ 
void parallel_sort_tests() {
  string tablename = "CILKsort";
  cout<<"\\def \\"<<tablename<<"blocksize {"<<BLOCK_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"numtrials {"<<NUM_TRIALS<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"maxinputsize {"<<MAX_INPUT_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"table {"<<endl;
  cout<<"\\begin{tikzpicture}[scale = .8]"
      <<endl<<"\\begin{axis}["
      <<endl<<"width = 5 in,"
      <<endl<<"height = 4in,"
      <<endl<<"title={Speedup Versus Number of Threads},"
      <<endl<<"xtick pos=left,"
      <<endl<<"ytick pos=left,"
      <<endl<<"legend style={draw=none},"
      <<endl<<"axis line style = { draw = none },"
      <<endl<<"legend pos= north west,"
      <<endl<<"xtick = data,"
      <<endl<<"xlabel={Number of Threads},"
      <<endl<<"ylabel={Speedup Over Serial Partition},"
      <<endl<<"ymax = 6,"
      <<endl<<"legend columns = 2,"
      <<endl<<"scatter/classes=%"
      <<endl<<"{a={mark=o,draw=blue}}]"<<endl;
  for (int64_t log_size = 24; (1 << log_size) <= MAX_INPUT_SIZE; log_size+=2) {
    double serial_baseline = test_sort_multiple_trials(0, (1 << log_size), NUM_TRIALS, 1);
    for (int64_t i = 1; i <= 2; i++) {
      if (i == 1) cout<<"%% Low Space with log size "<<log_size<<endl;
      if (i == 2) cout<<"%% High-Span with log size "<<log_size<<endl;
      cout<<"\\addplot coordinates {";
      for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
        set_num_threads(num_cores);
        double millisecs = test_sort_multiple_trials(i, (1 << log_size), NUM_TRIALS, num_cores);
        double speedup = serial_baseline / millisecs;
        cout << "( " << num_cores  << ", " << speedup << ") ";
      }
    }
    cout<<"};"<<endl;
  }
  cout<<"\\legend{ Low-Space 24, Low-Space 26, Low-Space 28, Two-Layer 24, Two-Layer 26, Two-Layer 28";
  cout<<"}"<<endl;
  cout<<"\\end{axis}"<<endl<<"\\end{tikzpicture}"<<endl<<"}"<<endl;
}


// Tests correctness of libc_quicksort implementation
void test_libc_quicksort(uint64_t n) {
  int64_t* array = (int64_t*) malloc(sizeof(int64_t) * n);
  for (int64_t i = 0; i < n; i++) array[i] = (int64_t)rand();
  libc_quicksort(array, n);
  for (int64_t i = 0; i < n - 1; i++) assert(array[i] <= array[i + 1]);
  free(array);
}

// Tests correctness of parallel_quicksort implementations.
void test_parallel_quicksort(uint64_t n) {
  int64_t* array = (int64_t*) malloc(sizeof(int64_t) * n);
  for (int64_t i = 0; i < n; i++) array[i] = (int64_t)rand();
  parallel_quicksort(array, n);
  for (int64_t i = 0; i < n - 1; i++) assert(array[i] <= array[i + 1]);
  free(array);

  array = (int64_t*) malloc(sizeof(int64_t) * n);
  for (int64_t i = 0; i < n; i++) array[i] = (int64_t)rand();
  parallel_quicksort_high_span(array, n);
  for (int64_t i = 0; i < n - 1; i++) assert(array[i] <= array[i + 1]);
  free(array);
}


// Runs parallel partition with parallel initialization, and without
// checks at end. Useful for things like measuring span.
void run_parallel_partition (uint64_t size) {
  gettimeofday(&tp, NULL);
  long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  int64_t* array = (int64_t*) malloc(sizeof(int64_t) * size);
  parallel_for (uint64_t i = 0; i < size / BLOCK_SIZE; i+=BLOCK_SIZE) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      array[i + j] = (i + j) % 100;
    }
  }

  //partition(array, size, 50);
  //small_prefix_partition(array, size, 50);
  in_place_partition(array, size, 50);
  free(array);

  gettimeofday(&tp, NULL);
  long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  uint64_t answer = (ms2 - ms1);
  cout<<answer<<" ms for partition"<<endl;
}

// This is for use with cachegrind to count cache misses. If
// just_initialization is true, then we perform only the
// initialization part of the computation without the actual partition
// step; this is so that we can subtract the number of cache misses
// for that from the full number of cache misses.
void run_parallel_partition_for_cache_misses (uint64_t in_place, uint64_t size, bool just_initialization) {
  int64_t* array = (int64_t*) malloc(sizeof(int64_t) * size);
  for (uint64_t i = 0; i < size; i++) {
    array[i] = rand();
  }
  if (!just_initialization) {
    if (in_place == 0) in_place_partition(array, size, rand());
    if (in_place == 1) small_prefix_partition(array, size, rand());
    if (in_place == 2) partition(array, size, rand());
  } else {
    if (array[rand() % size] == 11) {
      cout<<"This print statement is just to keep the initialization for loop from being optimized away"<<endl;
    }
  }
  free(array);
}

// Runs parallel sort with parallel initialization, and without
// checks at end. Useful for things like measuring span.
void run_parallel_qsort (uint64_t size) {
  gettimeofday(&tp, NULL);
  long int ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  int64_t* array = (int64_t*) malloc(sizeof(int64_t) * size);
  parallel_for (uint64_t i = 0; i < size; i++) {
    array[i] = rand();
  }
  parallel_quicksort(array, size);
  free(array);

  gettimeofday(&tp, NULL);
  long int ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  uint64_t answer = (ms2 - ms1);
  cout<<answer<<" ms for qsort"<<endl;
}

// Measures the sequential memory bandwidth of the machine using
// num_threads threads. Depending on arguments, either measures
// bandwidth for reads, writes, or for reads followed by
// writes. Returns answer in GB / sec.
double sequential_access_bandwidth (uint64_t num_threads, bool reads, bool writes) {
  uint64_t n = ((uint64_t)1 << 30);
  set_num_threads(num_threads);
  uint64_t chunk_size = n / (8 * num_threads);

  int64_t* array = (int64_t*) malloc(sizeof(int64_t) * n);
  for (uint64_t i = 0; i < n; i++) array[i] = i;
  gettimeofday(&tp, NULL);
  uint64_t ms1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  parallel_for (uint64_t i = 0; i < 8 * num_threads; i++) {
    uint64_t silly_sum = 0;
    for (uint64_t j = i * chunk_size; j < i * chunk_size + chunk_size; j++) {
      if (!reads) silly_sum++;
      else silly_sum += array[j];
      if (writes) array[j] = silly_sum;
    }
    if (silly_sum + array[11] == 139313) cout<<"This is so that the above loop doesn't get optimized out"<<endl;
  }
  gettimeofday(&tp, NULL);
  uint64_t ms2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  free(array);
  return  (double)n * 8 / ((double)(ms2 - ms1) / 1000) / pow(10, 9);
}

// Compares performance of in-place algorithm to best possible
// performance given the number of reads and writes that must be
// performed for the in-place algorithm to complete.
void bandwidth_bound_tests() {
  string tablename = "partitionbandwidthbound";
  double serial_baseline = test_partition_multiple_trials(-1, MAX_INPUT_SIZE, NUM_TRIALS, false, 1);
  cout<<"\\def \\"<<tablename<<"serialbaseline {"<<serial_baseline<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"blocksize {"<<BLOCK_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"numtrials {"<<NUM_TRIALS<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"inputsize {"<<MAX_INPUT_SIZE<<"}"<<endl;
  cout<<"\\def \\"<<tablename<<"table {"<<endl;
  cout<<"\\begin{tikzpicture}[scale = .8]"
      <<endl<<"\\begin{axis}["
      <<endl<<"width = 5 in,"
      <<endl<<"height = 4in,"
      <<endl<<"title={Speedup Versus Number of Threads},"
      <<endl<<"xtick pos=left,"
      <<endl<<"ytick pos=left,"
      <<endl<<"legend style={draw=none},"
      <<endl<<"axis line style = { draw = none },"
      <<endl<<"legend pos= north west,"
      <<endl<<"xtick = data,"
      <<endl<<"xlabel={Number of Threads},"
      <<endl<<"ylabel={Speedup Over Serial Partition},"
      <<endl<<"ymax = 4,"
      <<endl<<"legend columns = 2,"
      <<endl<<"scatter/classes=%"
      <<endl<<"{a={mark=o,draw=blue}}]"<<endl;
  
  cout<<"%% In-Place"<<endl;
  cout<<"\\addplot coordinates {";
  for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
    double millisecs = test_partition_multiple_trials(0, MAX_INPUT_SIZE, NUM_TRIALS, false, num_cores);
    double speedup = serial_baseline / millisecs;
    cout << "( " << num_cores  << ", " << speedup << ") ";
  }
  cout<<"};"<<endl;
  
  cout<<"%% Low-Space Bandwidth Bound"<<endl;
  cout<<"\\addplot coordinates {";
  for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
    double av_best_speedup = 0;
    for (int64_t trial = 0; trial  < NUM_TRIALS; trial++) {
      double read_bandwidth = sequential_access_bandwidth(num_cores, true, false);
      double both_bandwidth = sequential_access_bandwidth(num_cores, true, true);
      double num_bytes_read_and_written = MAX_INPUT_SIZE * sizeof(int64_t) * 3.5;
      double num_bytes_just_read =  MAX_INPUT_SIZE * sizeof(int64_t) * 0.5;
      // The next two lines account for the fact that a small fraction
      // of reads and writes are by sheer luck already in cache; the
      // fraction is as determined using cachegrind.
      //num_bytes_read_and_written *=  124.2 / 134.3;
      //num_bytes_just_read *=  124.2 / 134.3;
      double required_runtime_for_read_writes =
	num_bytes_read_and_written  / (both_bandwidth * pow(10.0, 9));
      double required_runtime_for_just_reads =
	num_bytes_just_read / (read_bandwidth * pow(10.0, 9));
      double total_required_runtime =
	required_runtime_for_read_writes + required_runtime_for_just_reads;
      double best_speedup = serial_baseline / (total_required_runtime * 1000.0);
      av_best_speedup += best_speedup;
    }
    av_best_speedup /= NUM_TRIALS;
    cout << "(" << num_cores <<", "<< av_best_speedup <<")";
  }
  cout<<"};"<<endl;
  
  cout<<"%% high span"<<endl;
  cout<<"\\addplot coordinates {";
  for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
    double millisecs = test_partition_multiple_trials(3, MAX_INPUT_SIZE, NUM_TRIALS, false, num_cores);
    double speedup = serial_baseline / millisecs;
    cout << "( " << num_cores  << ", " << speedup << ") ";
  }
  cout<<"};"<<endl;

  cout<<"%% High-Span Bandwidth Bound"<<endl;
  cout<<"\\addplot coordinates {";
  for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
    double av_best_speedup = 0;
    for (int64_t trial = 0; trial  < NUM_TRIALS; trial++) {
      double read_bandwidth = sequential_access_bandwidth(num_cores, true, false);
      double both_bandwidth = sequential_access_bandwidth(num_cores, true, true);
      double num_bytes_read_and_written = MAX_INPUT_SIZE * sizeof(int64_t) * 2;
      double num_bytes_just_read =  0;
      // The next two lines account for the fact that a small fraction
      // of reads and writes are by sheer luck already in cache; the
      // fraction is as determined using cachegrind.
      //num_bytes_read_and_written *=  124.2 / 134.3;
      //num_bytes_just_read *=  124.2 / 134.3;
      double required_runtime_for_read_writes =
	num_bytes_read_and_written  / (both_bandwidth * pow(10.0, 9));
      double required_runtime_for_just_reads =
	num_bytes_just_read / (read_bandwidth * pow(10.0, 9));
      double total_required_runtime =
	required_runtime_for_read_writes + required_runtime_for_just_reads;
      double best_speedup = serial_baseline / (total_required_runtime * 1000.0);
      av_best_speedup += best_speedup;
    }
    av_best_speedup /= NUM_TRIALS;
    cout << "(" << num_cores <<", "<< av_best_speedup <<")";
  }

  cout<<"%% cache friendly"<<endl;
  cout<<"\\addplot coordinates {";
  for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
    double millisecs = test_partition_multiple_trials(4, MAX_INPUT_SIZE, NUM_TRIALS, false, num_cores);
    double speedup = serial_baseline / millisecs;
    cout << "( " << num_cores  << ", " << speedup << ") ";
  }
  cout<<"};"<<endl; 

  cout<<"%% Cache-Friendly Bandwidth Bound"<<endl;
  cout<<"\\addplot coordinates {";
  for (int64_t num_cores = 1; num_cores <= NUM_THREADS_DEFAULT; num_cores++) {
    double av_best_speedup = 0;
    for (int64_t trial = 0; trial  < NUM_TRIALS; trial++) {
      double read_bandwidth = sequential_access_bandwidth(num_cores, true, false);
      double both_bandwidth = sequential_access_bandwidth(num_cores, true, true);
      double num_bytes_read_and_written = MAX_INPUT_SIZE * sizeof(int64_t) * 1;
      double num_bytes_just_read =  0;
      // The next two lines account for the fact that a small fraction
      // of reads and writes are by sheer luck already in cache; the
      // fraction is as determined using cachegrind.
      //num_bytes_read_and_written *=  124.2 / 134.3;
      //num_bytes_just_read *=  124.2 / 134.3;
      double required_runtime_for_read_writes =
	num_bytes_read_and_written  / (both_bandwidth * pow(10.0, 9));
      double required_runtime_for_just_reads =
	num_bytes_just_read / (read_bandwidth * pow(10.0, 9));
      double total_required_runtime =
	required_runtime_for_read_writes + required_runtime_for_just_reads;
      double best_speedup = serial_baseline / (total_required_runtime * 1000.0);
      av_best_speedup += best_speedup;
    }
    av_best_speedup /= NUM_TRIALS;
    cout << "(" << num_cores <<", "<< av_best_speedup <<")";
  }


  
  cout<<"};"<<endl;
  cout<<"\\legend{Low-Space, Low-Space Bandwidth Constraint, Two-Layer, Two-Layer Bandwidth Constraint}"<<endl;
  cout<<"\\end{axis}"<<endl<<"\\end{tikzpicture}"<<endl<<"}"<<endl;
}

int main() {
  srand (time(NULL)); // initialize random seed
  // BLOCK_SIZE needs to be a power of two:
  assert(((BLOCK_SIZE - 1) & BLOCK_SIZE) == 0);
  // cout<<test_partition(-1, (1 << 28), false, NUM_THREADS_DEFAULT)<<endl;
  //cout<<test_partition_multiple_trials(3, (1 << 24), NUM_TRIALS, false, 1)<<endl;
  //cout<<test_sort(1, (1<<28), NUM_THREADS_DEFAULT)<<endl;
  //cout<<test_sort(2, (1<<28), NUM_THREADS_DEFAULT)<<endl;
  //return 0;
  // To do cachegrind tests, uncomment the following two lines.  Then
  // run with cachegrind.  Then change the boolean to true and run
  // again with cachegrind.  Subtract the number of cache misses in
  // the second run from those in the first in order to determine the
  // true number of cache misses that can be attributed to the
  // parallel partition. Change the first argument to change which
  // partition function you're testing.
  // run_parallel_partition_for_cache_misses (0, (1 << 28), false);
  //return 0;
  
  // We begin by running tests of correctness on the functions
  test_prefix_sum();
  test_libc_quicksort((1<<20));
  test_sort(1, (1<<20), NUM_THREADS_DEFAULT);
  test_partition(-2, 141123, false, NUM_THREADS_DEFAULT); // Make sure to test on a non-power of two here for completeness.
  test_partition(-1, 141123, false, NUM_THREADS_DEFAULT); 
  test_partition(0, 141123, false, NUM_THREADS_DEFAULT);
  test_partition(0, 141123, true, NUM_THREADS_DEFAULT); // This is the case where the value of more_succ changes which case of the code is run
  test_partition(1, 141123, false, NUM_THREADS_DEFAULT);
  test_partition(2, 141123, false, NUM_THREADS_DEFAULT);
  test_partition(3, 141123, false, NUM_THREADS_DEFAULT);
  test_partition(4, 141123, false, NUM_THREADS_DEFAULT);
  cout << "% ran the tests " << endl;

  uint64_t size_to_run_on = (1 << 29);
  uint64_t num_threads = 1;

   
  // cout<<"High-Span Algorithm: (size "<<size_to_run_on<<", num_threads "<<num_threads<<")"<<endl;
  // cout<<test_partition(3, size_to_run_on, false, num_threads)<<endl;
  // cout<<"Cache Friendly Algorithm: (size "<<size_to_run_on<<", num_threads "<<num_threads<<")"<<endl;
  // cout<<test_partition(4, size_to_run_on, false, num_threads)<<endl;

 
  // cout<<"Our Serial Partition: (size "<<size_to_run_on<<")"<<endl;
  // cout<<test_partition(-3, size_to_run_on, false, num_threads)<<endl;
  // cout<<"LibC Serial Partition: (size "<<size_to_run_on<<")"<<endl;
  // cout<<test_partition(-1, size_to_run_on, false, num_threads)<<endl;


  for (size_to_run_on = (1 << 23); size_to_run_on *= 2; size_to_run_on <= (1 << 30)) {
    cout<<"----------------------------------------"<<endl;
    num_threads = 18;
    cout<<"High-Span Algorithm: (size "<<size_to_run_on<<", num_threads "<<num_threads<<")"<<endl;
    cout<<test_partition(3, size_to_run_on, false, num_threads)<<endl;
    cout<<"Cache Friendly Algorithm: (size "<<size_to_run_on<<", num_threads "<<num_threads<<")"<<endl;
    cout<<test_partition(4, size_to_run_on, false, num_threads)<<endl;
    
    cout<<"%% Cache-Friendly Bandwidth Bound"<<endl;
    int64_t num_cores = num_threads;
    double read_bandwidth = sequential_access_bandwidth(num_cores, true, false);
    double both_bandwidth = sequential_access_bandwidth(num_cores, true, true);
    double num_bytes_read_and_written = size_to_run_on * sizeof(int64_t) * 1;
    double num_bytes_just_read =  0;
    // The next two lines account for the fact that a small fraction
    // of reads and writes are by sheer luck already in cache; the
    // fraction is as determined using cachegrind.
    //num_bytes_read_and_written *=  124.2 / 134.3;
    //num_bytes_just_read *=  124.2 / 134.3;
    double required_runtime_for_read_writes =
      num_bytes_read_and_written  / (both_bandwidth * pow(10.0, 9));
    double required_runtime_for_just_reads =
      num_bytes_just_read / (read_bandwidth * pow(10.0, 9));
    double total_required_runtime =
      required_runtime_for_read_writes + required_runtime_for_just_reads;
    cout<<(int) (total_required_runtime * 1000)<<endl;
  }

  return 0;

  

#ifdef USE_CILK
  cout << "starting parallel test 1" << endl;
  parallel_partition_tests(); // Run the parallel tests!
  cout << "starting parallel test 2" << endl;
  parallel_partition_tests_two(); // Run the parallel tests!
  parallel_sort_tests(); 
  bandwidth_bound_tests();
#endif
#ifdef RUN_SERIAL
  serial_partition_tests(); // Run the serial tests!
#endif
  // This is just some code I like to run when messing around with things:
  // set_num_threads(2);
  //cout<<test_partition(-1, n, false)<<endl;
  //cout<<test_partition(-2, n, false)<<endl;
  //cout<<test_partition(0, n, false)<<endl;
  // cout<<test_partition(0, n, true)<<endl;
  //cout<<test_partition(1, n, false)<<endl;
  //cout<<test_partition(2, n, false)<<endl;
  return 0;
}
