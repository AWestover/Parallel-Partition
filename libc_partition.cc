#include "libc_partition.h"
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
#include <string>
#include <alloca.h>
#include <limits.h>


using namespace std;

/*
 *  Copies the partition function used in libc.  See
 *  https://github.com/lattera/glibc/blob/master/stdlib/qsort.c The
 *  only modifications are: we use only the portion of the code used
 *  for partitioning, and use the fact that we are partitioning
 *  integers to replace byte-wise moves of objects with simply moving
 *  around integers, and to replace function-call comparisions with
 *  standard integer comparison. Returns number of elements in left
 *  part of partition.
 */
uint64_t libc_partition(int64_t *array, uint64_t n, int64_t pivot) {
  if (n == 0) return 0;
  int64_t *left_ptr = array;
  int64_t *right_ptr = array + n - 1;
  do
    {
      while (*left_ptr < pivot)
        left_ptr ++;
      while (*right_ptr > pivot)
        right_ptr --;
      
      if (left_ptr < right_ptr)
        {
          int64_t tmp = *left_ptr;
          *left_ptr = *right_ptr;
          *right_ptr = tmp;
          left_ptr ++;
          right_ptr --;
        }
      else if (left_ptr == right_ptr)
        {
          left_ptr ++;
          right_ptr --;
          break;
        }
    }
  while (left_ptr <= right_ptr);
  return left_ptr - array;
}

// Like libc_partition but places all elements <= pivot on one side.
uint64_t libc_partition_strict(int64_t *array, uint64_t n, int64_t pivot) {
  if (n == 0) return 0;
  int64_t *left_ptr = array;
  int64_t *right_ptr = array + n - 1;
  do
    {
      while (*left_ptr <= pivot)
        left_ptr ++;
      while (*right_ptr > pivot)
        right_ptr --;
      
      if (left_ptr < right_ptr)
        {
          int64_t tmp = *left_ptr;
          *left_ptr = *right_ptr;
          *right_ptr = tmp;
          left_ptr ++;
          right_ptr --;
        }
      else
	{
	  break;
	}
    }
  while (left_ptr <= right_ptr);
  return left_ptr - array;
}




/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
  {
    int64_t *lo;
    int64_t *hi;
  } stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define STACK_SIZE	(CHAR_BIT * sizeof(uint64_t))
#define PUSH(low, high)	((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define	POP(low, high)	((void) (--top, (low = top->lo), (high = top->hi)))
#define	STACK_NOT_EMPTY	(stack < top)

/*
 *  Copies the sort function used in libc.  See
 *  https://github.com/lattera/glibc/blob/master/stdlib/qsort.c The
 *  only modifications are: we use only the portion of the code used
 *  for partitioning, and use the fact that we are partitioning
 *  integers to replace byte-wise moves of objects with simply moving
 *  around integers, and to replace function-call comparisions with
 *  standard integer comparison.
 */

void libc_quicksort (void *const pbase, size_t total_elems)
{
  int64_t *base_ptr = (int64_t *) pbase;
  const size_t max_thresh = MAX_THRESH;
  if (total_elems == 0)
    /* Avoid lossage with unsigned arithmetic below.  */
    return;
  
  if (total_elems > MAX_THRESH) {
    int64_t *lo = base_ptr;
    int64_t *hi = &lo[total_elems - 1];
    stack_node stack[STACK_SIZE];
    stack_node *top = stack;

    PUSH (NULL, NULL);
    
    while (STACK_NOT_EMPTY){
      int64_t *left_ptr;
      int64_t *right_ptr;
      
      /* Select median value from among LO, MID, and HI. Rearrange
         LO and HI so the three values are sorted. This lowers the
         probability of picking a pathological pivot value and
	     skips a comparison for both the LEFT_PTR and RIGHT_PTR in
	     the while loops. */
      int64_t *mid = lo + ((hi - lo) >> 1);

      /* Using strict < and > for the next two if statements is
       *  actually _very_ important. It ensures that if almost all the
       *  elements = the pivot, we still partition roughly equally.
       */

      if (*mid < *lo) {
        int64_t tmp = *mid;
        *mid = *lo;
        *lo = tmp;
      }
      if (*hi < *mid) {
        int64_t tmp = *mid;
        *mid = *hi;
        *hi = tmp;
          } else {
        goto jump_over;
      }
      if (*mid < *lo) {
        int64_t tmp = *mid;
        *mid = *lo;
        *lo = tmp;
      }
    jump_over:;
      
      int64_t pivot = *mid;
      
      left_ptr  = lo + 1;
      right_ptr = hi - 1;
      do {
        while (*left_ptr < pivot)
          left_ptr ++;
        while (*right_ptr > pivot)
          right_ptr --;
        
        if (left_ptr < right_ptr) {
          int64_t tmp = *left_ptr;
          *left_ptr = *right_ptr;
          *right_ptr = tmp;
          left_ptr ++;
          right_ptr --;
        } else if (left_ptr == right_ptr) {
          left_ptr ++;
          right_ptr --;
          break;
        }
      }  while (left_ptr <= right_ptr);
      /* Set up pointers for next iteration.  First determine whether
         left and right partitions are below the threshold size.  If so,
         ignore one or both.  Otherwise, push the larger partition's
         bounds on the stack and continue sorting the smaller one. */
      
      if ((size_t) (right_ptr - lo) <= max_thresh) {
        if ((size_t) (hi - left_ptr) <= max_thresh)
          /* Ignore both small partitions. */
          POP (lo, hi);
        else
          /* Ignore small left partition. */
          lo = left_ptr;
      }
      else if ((size_t) (hi - left_ptr) <= max_thresh)
        /* Ignore small right partition. */
        hi = right_ptr;
      else if ((right_ptr - lo) > (hi - left_ptr)) {
        /* Push larger left partition indices. */
        PUSH (lo, right_ptr);
        lo = left_ptr;
      } else {
        /* Push larger right partition indices. */
        //cout<<(int)(stack - top)<<endl;
        PUSH (left_ptr, hi);
        hi = right_ptr;
      }
    }
  }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */

#define min(x, y) ((x) < (y) ? (x) : (y))

  {
    int64_t *const end_ptr = &base_ptr[(total_elems - 1)];
    int64_t *tmp_ptr = base_ptr;
    int64_t *thresh = min(end_ptr, base_ptr + max_thresh);
    int64_t *run_ptr;

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + 1; run_ptr <= thresh; run_ptr ++)
      if (*run_ptr < *tmp_ptr)
        tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr) {
      int64_t tmp = *tmp_ptr;
      *tmp_ptr = *base_ptr;
      *base_ptr = tmp;
    }
    /* Insertion sort, running from left-hand-side up to right-hand-side.  */

    run_ptr = base_ptr + 1;
    while ((run_ptr += 1) <= end_ptr) {
      tmp_ptr = run_ptr - 1;
      while (*run_ptr < *tmp_ptr)
        tmp_ptr --;

      tmp_ptr ++;
      if (tmp_ptr != run_ptr) {
        int64_t tmp = *run_ptr;
        int64_t* trav = tmp_ptr;
        while (trav <= run_ptr) {
          int64_t tmp2 = *trav;
          *trav = tmp;
          tmp = tmp2;
          trav++;
        }
      }
    }
  }
}


/*
 * We use the same pivot selection strategy as libc_qsort.
 */
int64_t select_pivot(int64_t* array, uint64_t num_elts) {
  int64_t* lo = array;
  int64_t *hi = array + num_elts - 1;
  int64_t *mid = lo + ((hi - lo) >> 1);
  if (*mid < *lo) {
    int64_t tmp = *mid;
    *mid = *lo;
    *lo = tmp;
  }
  if (*hi < *mid) {
    int64_t tmp = *mid;
    *mid = *hi;
    *hi = tmp;
  } else {
    goto jump_over;
  }
  if (*mid < *lo) {
    int64_t tmp = *mid;
        *mid = *lo;
        *lo = tmp;
  }
 jump_over:;    
 return *mid;
}

