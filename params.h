/* 
 *  Sets parameters used in main.cc. Some of these, such as whether we
 *  wish to compile in serial versus parallel must be hardcoded via
 *  macros. The others could just be normal variables.
 */

#include <sys/time.h>
#include <cstdint>
#include <cilk/cilk.h>
#include <string>

#ifndef PARAMS_H
#define PARAMS_H

// NUM_TRIALS is the number of trials tests are averaged over
#define NUM_TRIALS 5
// BLOCK_SIZE is the value that is used as log n when performing optimizations on parallel prefix sum
// Must be a power of two. Interestingly, reducing this doesn't change CILK's measured span.
#define BLOCK_SIZE 64
// MAX_INPUT_SIZE governs the size of the maximum array we will test on
#define MAX_INPUT_SIZE (1 << 30)
// NUM_THREADS_DEFAULT should be set to the number of cores on the machine we're running on
#define NUM_THREADS_DEFAULT 18
// Exactly one of USE_CILK and RUN_SERIAL should be commented.
#define USE_CILK
//#define RUN_SERIAL


#ifdef USE_CILK
#define parallel_for cilk_for
#define parallel_spawn cilk_spawn
#define parallel_sync cilk_sync
#endif
#ifdef RUN_SERIAL
#define parallel_for for
#define parallel_spawn
#define parallel_sync
#endif

#endif
