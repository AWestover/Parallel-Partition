
// AUTHOR: Alek Westover
// cache friendly parallel partition program
// note that this code uses our algorithm in the recursive step for parallel partition

// break the array into s chunks
// make g = n /(b*s) groups, with elements determined in the following manner:
// on each chunk, generate 1 random number,  
// X[i] + i % (num cache block chunks in the chunk of the array) is the index into the chunk of the array that we are considering of the cache block that is added to group i from this chunk of the array
// serial partition each group (we can do all the serial partitions for different groups in parallel)
// in each group compute vi: the middle (where the array switches from having predecessors to having successors, equivalently, compute the number of predecessors in the array)
// we now recurse on min(vi) to max(vi) with a new appropriately chosen group size and number of groups for the smaller array

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
using namespace std;

// if equal delta = true, then we 
#define equalDelta false
#define multiplicationInLoop false

struct Vs {
	int64_t vmin; 
	int64_t vmax;
};

// s = log(n) / (2*delta*delta)
// g = n / (b*s)
// smaller delta -> bigger s -> smaller g
// equalDelta false -> use small delta = 1 / sqrt(2*log(n)) on the top
// so equalDelta false means that there will be fewer groups on the top layer (and that the groups will be bigger therefore, so we can be confident that the recursive subproblem is much smaller in size)

int64_t computeS(int64_t n, double delta){
    return (int64_t)(log2(n) / (2*delta*delta));
}

// we partition sufficiently small arrays in serial, because at this point the overhead is too big to make parallelizing it make sense 
int64_t serialPartitioner(int64_t* A, int64_t n, int64_t pivotVal) {
    int64_t low = 0; int64_t high = n-1;
    while(low < high){
        while(A[low] <= pivotVal && low < high) { // we have to check low < high because otherwise [1 2 3 4 5] with pivot value 10 would cause a problem
            low++;
        }
        while (A[high] > pivotVal && low < high){
            high--;
        }
        // swap
        int64_t tmp = A[low];
        A[low] = A[high];
        A[high] = tmp;
		// we know already that 
		// A[low] <= pivotVal, A[high] > pivotVal, this saves us 1 check of the while loops above
		low++; high--;
    }
	low--; // shouldn't have incremented on the last itteration of the while loop, this neutralizes the final low++
    if(A[low] <= pivotVal){
        low++;
    }
    return low;
}

// this is our implementation of a parallel for loops with spawning recursive subproblems
// it outputs vmin, vmax
Vs serialGroupPartitions(int64_t i, int64_t groupsLeftLoop, int64_t* A, int64_t pivotVal, int64_t* X, int64_t s, int64_t logB, int64_t b, int64_t g, int64_t XdeltaAdelta){
	if (groupsLeftLoop == 1){
		// Xidx indicates which chunk low is currently in
		int64_t lowXidx = 0; // index into X
		// on each chunk of the array, each group gets a portion of size b, which is determined by X[i] and i, specifically X[i]+i % g is the index of the chunk of b that you get
		// offset tells you X[i]+i % g, which is the index into the chunk of assigned to this group
		int64_t lowXoffset = (X[lowXidx] + i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i; // cheaper than modulus

		// Xidx indicates which chunk low is currently in
		int64_t highXidx = s-1; // index into X
		// on each chunk of the array, each group gets a portion of size b, which is determined by X[i] and i, specifically X[i]+i % g is the index of the chunk of b that you get
		// offset tells you X[i]+i % g, which is the index into the chunk of assigned to this group
		int64_t highXoffset = (X[highXidx]+i >= g) ? X[highXidx]+i - g : X[highXidx]+i; // cheaper than modulus

		// ALowIdx refers to the actual index in the array A that low is on
		int64_t ALowIdx = (lowXoffset+g*lowXidx) << logB;
		// we will increment ALowIdx within the block of size b until it reaches the end of the block, at block_boundary_low
		int64_t block_boundary_low = ALowIdx + b;

		// block_boundary high is 1 less than the end of the block that we are currently in. we will decrement AHighIdx until it reaches the boundary
		int64_t block_boundary_high =  ((highXoffset+g*highXidx)<<logB) - 1;
		// AHighIdx refers to the actual index in the array A that high is on
		int64_t AHighIdx = block_boundary_high + b;
	  
		// similar to the serial partition loop, (we are processing each group in serial, in parallel over the groups) but it differs
		// in that the next element function is trickier than just +1 because we must jump blocks whenever we reach a block boundary
		// that is the if statement, which implements jumping to the next block 
		while(ALowIdx < AHighIdx){
			while(A[ALowIdx] <= pivotVal && ALowIdx < AHighIdx) {
				ALowIdx++;
				if(ALowIdx == block_boundary_low){
					// jump to the next chunk of the array
					lowXidx += 1;
					// this is a version without multiplicatoin, it may be slower thouhg, because it has extra add and subtract steps, and an extra variable
					if(!multiplicationInLoop) {
						int64_t deltaLowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i;
						deltaLowXoffset -= lowXoffset; 
						lowXoffset += deltaLowXoffset;
						ALowIdx += (deltaLowXoffset<<logB) + XdeltaAdelta - b;
					}
					else {
						// get the position within the chunk 
						lowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i; // cheaper than modulus 
						// get the start of the chunk
						ALowIdx = (lowXoffset+g*lowXidx) << logB;
					}
					// get the end of the chunk
					block_boundary_low = ALowIdx + b; 
				}
			}
			while (A[AHighIdx] > pivotVal && ALowIdx < AHighIdx){
				AHighIdx--;
				if(AHighIdx == block_boundary_high){
					// jump to the next chunk of the array (going backwards)
					highXidx -= 1;
					// this is a version without multiplicatoin, it may be slower thouhg, because it has extra add and subtract steps, and an extra variable
					if(!multiplicationInLoop) {
						int64_t deltaHighXoffset = (X[highXidx]+i >= g) ? X[highXidx]+i - g : X[highXidx]+i;
						deltaHighXoffset -= highXoffset; 
						highXoffset += deltaHighXoffset;
						block_boundary_high += (deltaHighXoffset<<logB) - XdeltaAdelta;
					}
					else {
						// get the position within the chunk 
						highXoffset = (X[highXidx]+i >= g) ? X[highXidx]+i - g : X[highXidx]+i; // cheaper than modulus
						// get the start of the chunk (where we end because we are going backwards)
						block_boundary_high = ((highXoffset+g*highXidx) << logB) - 1;                 
					}
					// get the end of the chunk (where we start because we are going backwards)
					AHighIdx =  block_boundary_high + b;
				}
			}
			// swap the elements
			int64_t tmp = A[ALowIdx];
			A[ALowIdx] = A[AHighIdx];
			A[AHighIdx] = tmp;
		}
		// make sure that lowIdx specifies the number of predecessors and not number of predecessors - 1  (either are possible until we do this cleanup step)
		if(A[ALowIdx] <= pivotVal){
			// this is the exact same incrementing code as above
			ALowIdx++;
			if(ALowIdx == block_boundary_low){
				lowXidx += 1;
				if(!multiplicationInLoop) {
					int64_t deltaLowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i;
					deltaLowXoffset -= lowXoffset; 
					lowXoffset += deltaLowXoffset;
					ALowIdx += (deltaLowXoffset<<logB) + XdeltaAdelta - b;
				}
				else {
					lowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i; // cheaper than modulus 
					ALowIdx = (lowXoffset+g*lowXidx) << logB;
				}
				block_boundary_low = ALowIdx + b;
			}
		}

		Vs vscur;
		vscur.vmin = ALowIdx;
		vscur.vmax = ALowIdx;
		return vscur;
	}
	else{
		Vs vsleft = parallel_spawn serialGroupPartitions(i, groupsLeftLoop/2, A, pivotVal, X, s, logB, b, g, XdeltaAdelta);
		Vs vsright = serialGroupPartitions(i+groupsLeftLoop/2, groupsLeftLoop/2+(groupsLeftLoop&1), A, pivotVal, X, s, logB, b, g, XdeltaAdelta);
		parallel_sync;
		Vs vsaggregate;
		vsaggregate.vmin = std::min(vsleft.vmin, vsright.vmin);
		vsaggregate.vmax = std::max(vsleft.vmax, vsright.vmax);
		return vsaggregate;
	}
}

// this function is the most important part of this file
// it implements the grouped partition, and will be called from a different function that can implement special behavior on the top layer
int64_t groupedPartitionRecursive(int64_t* A, int64_t n, int64_t pivotVal, int64_t s, int64_t logB, double delta){
    int64_t b = 1<<logB; // cache block size 
    int64_t g = n / (b*s);  // g
    if (g > 2){
        // the array A has been broken up into s parts, and each index into the array X denotes something about one of these parts
        // the size of each of the s parts is g*b = XdeltaAdelta
        int64_t XdeltaAdelta = g<<logB;
        
        int64_t* X = (int64_t*)malloc(sizeof(int64_t)*s);
        // generate X (one list to store them all)
        for (int64_t i = 0; i < s; ++i) {
            X[i] = rand() % g;
        }

		/**/
        // serial partition on each group done in parallel across the groups
		int64_t* allVs = (int64_t*)malloc(sizeof(int64_t)*g);

        // everything with idx < vmin has A[idx] <= pivotval
        // everything with idx >= vmax has A[idx] > pivotVal
        // so the thing to recurse on is A+vmin, with size vmax-vmin
        
        // parallel partition takes the most (nearly all) time in this loop on the top level of recursion
        parallel_for (int64_t i = 0; i < g; ++i) {

			// Xidx indicates which chunk low is currently in
            int64_t lowXidx = 0; // index into X
			// on each chunk of the array, each group gets a portion of size b, which is determined by X[i] and i, specifically X[i]+i % g is the index of the chunk of b that you get
			// offset tells you X[i]+i % g, which is the index into the chunk of assigned to this group
            int64_t lowXoffset = (X[lowXidx] + i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i; // cheaper than modulus

			// Xidx indicates which chunk low is currently in
            int64_t highXidx = s-1; // index into X
			// on each chunk of the array, each group gets a portion of size b, which is determined by X[i] and i, specifically X[i]+i % g is the index of the chunk of b that you get
			// offset tells you X[i]+i % g, which is the index into the chunk of assigned to this group
            int64_t highXoffset = (X[highXidx]+i >= g) ? X[highXidx]+i - g : X[highXidx]+i; // cheaper than modulus

			// ALowIdx refers to the actual index in the array A that low is on
            int64_t ALowIdx = (lowXoffset+g*lowXidx) << logB;
			// we will increment ALowIdx within the block of size b until it reaches the end of the block, at block_boundary_low
            int64_t block_boundary_low = ALowIdx + b;

			// block_boundary high is 1 less than the end of the block that we are currently in. we will decrement AHighIdx until it reaches the boundary
            int64_t block_boundary_high =  ((highXoffset+g*highXidx)<<logB) - 1;
			// AHighIdx refers to the actual index in the array A that high is on
            int64_t AHighIdx = block_boundary_high + b;
          
			// similar to the serial partition loop, (we are processing each group in serial, in parallel over the groups) but it differs
			// in that the next element function is trickier than just +1 because we must jump blocks whenever we reach a block boundary
			// that is the if statement, which implements jumping to the next block 
            while(ALowIdx < AHighIdx){
                while(A[ALowIdx] <= pivotVal && ALowIdx < AHighIdx) {
                    ALowIdx++;
                    if(ALowIdx == block_boundary_low){
						// jump to the next chunk of the array
                        lowXidx += 1;
						// this is a version without multiplicatoin, it may be slower thouhg, because it has extra add and subtract steps, and an extra variable
						if(!multiplicationInLoop) {
							int64_t deltaLowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i;
							deltaLowXoffset -= lowXoffset; 
							lowXoffset += deltaLowXoffset;
							ALowIdx += (deltaLowXoffset<<logB) + XdeltaAdelta - b;
						}
						else {
							// get the position within the chunk 
                            lowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i; // cheaper than modulus 
							// get the start of the chunk
							ALowIdx = (lowXoffset+g*lowXidx) << logB;
						}
						// get the end of the chunk
						block_boundary_low = ALowIdx + b; 
                    }
                }
                while (A[AHighIdx] > pivotVal && ALowIdx < AHighIdx){
                    AHighIdx--;
                    if(AHighIdx == block_boundary_high){
						// jump to the next chunk of the array (going backwards)
                        highXidx -= 1;
						// this is a version without multiplicatoin, it may be slower thouhg, because it has extra add and subtract steps, and an extra variable
                        if(!multiplicationInLoop) {
							int64_t deltaHighXoffset = (X[highXidx]+i >= g) ? X[highXidx]+i - g : X[highXidx]+i;
                            deltaHighXoffset -= highXoffset; 
                            highXoffset += deltaHighXoffset;
                            block_boundary_high += (deltaHighXoffset<<logB) - XdeltaAdelta;
                        }
                        else {
							// get the position within the chunk 
                            highXoffset = (X[highXidx]+i >= g) ? X[highXidx]+i - g : X[highXidx]+i; // cheaper than modulus
							// get the start of the chunk (where we end because we are going backwards)
                            block_boundary_high = ((highXoffset+g*highXidx) << logB) - 1;                 
                        }
						// get the end of the chunk (where we start because we are going backwards)
                        AHighIdx =  block_boundary_high + b;
                    }
                }
				// swap the elements
                int64_t tmp = A[ALowIdx];
                A[ALowIdx] = A[AHighIdx];
                A[AHighIdx] = tmp;
            }
			// make sure that lowIdx specifies the number of predecessors and not number of predecessors - 1  (either are possible until we do this cleanup step)
            if(A[ALowIdx] <= pivotVal){
				// this is the exact same incrementing code as above
                ALowIdx++;
                if(ALowIdx == block_boundary_low){
					lowXidx += 1;
					if(!multiplicationInLoop) {
						int64_t deltaLowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i;
						deltaLowXoffset -= lowXoffset; 
						lowXoffset += deltaLowXoffset;
						ALowIdx += (deltaLowXoffset<<logB) + XdeltaAdelta - b;
					}
					else {
						lowXoffset = (X[lowXidx]+i >= g) ? X[lowXidx]+i - g : X[lowXidx]+i; // cheaper than modulus 
						ALowIdx = (lowXoffset+g*lowXidx) << logB;
					}
                    block_boundary_low = ALowIdx + b;
                }
            }
            allVs[i] = ALowIdx;
        }


        // find the minimum in serial (we had data races when we tried to in parallel have everything write to the same variable at the same time (concurrent writing))
        int64_t vmin = n-1; int64_t vmax = 0;
        for (int64_t i = 0; i < g; i++) {
            if(allVs[i] < vmin)
                vmin = allVs[i];
            if(allVs[i] > vmax)
                vmax = allVs[i];
        }
		/**/
		// Vs vsfinal = serialGroupPartitions(0, g, A, pivotVal, X, s, logB, b, g, XdeltaAdelta);
		// int64_t vmin = vsfinal.vmin;
		// int64_t vmax = vsfinal.vmax;

        // here is a bit of edge case cleanup
        // need to partition A from (s*b)*g (ie sb(n/(sb))) to n
        // there are 2 cases, based on whether ther are more extra elements than known sucessors or not

        // if there are less extra elements than sucessors (this is true on decenetly big inputs) then just sacrifice some of the successors by swapping them with extras
        int64_t leftOverStart = s*b*g;
        int64_t leftOverSize = n - leftOverStart;
        int64_t successorPartitionSize = leftOverStart - vmax;
        if (leftOverSize < successorPartitionSize) { // llllllllll ?????????? hhhhhhhhhhhhhhhhhhhhhhh  ???? swap the ?s with the early hs
            for(int64_t i = 0; i < leftOverSize; i++){
                int64_t tmp = A[leftOverStart + i];
                A[leftOverStart + i] = A[vmax];
                A[vmax] = tmp; 
                vmax += 1;
            }
        }
		// if there are less successors than extra elements (this can only happen on inredibly small inputs) then sacrifice the successors
        else { // llllllllll ?????????? hhhh ???????? swap hs with the back of the extra section
            for(int64_t i = 0; i < successorPartitionSize; i++){
                int64_t tmp = A[n - 1 - i];
                A[n - 1 - i] = A[vmax];
                A[vmax] = tmp; 
                vmax += 1;
            }
            vmax = n - successorPartitionSize;      
            assert (vmax < n); // this should never happen. It would indicate that the vmax code got messed up if it did. It would also indicate taht there are no successsors in the array. Oh wait, that is possible, dang, add a test case for this (adverserial pivotValue)
        }

        // solve the smaller subproblem recursively, which starts at vmin, and has size vmax - vmin
        free(X); // free the array! 
        int64_t middle = vmin + groupedPartitionRecursive(A+vmin, vmax-vmin, pivotVal, computeS(vmax-vmin, delta), logB, delta);

        return middle;
    }
    else { // if the input is really small, delegate to a serial partitioner
        return serialPartitioner(A, n, pivotVal);   
    }
}

// this is the top level, 
// because the top level takes up nearly all of the time, we can do special tuning for it here
int64_t groupedPartition(int64_t* A, int64_t N0, int64_t pivotVal){
    double delta = 0.5;
    int64_t logB = 9;

    int64_t logN0 = (int64_t)log2(N0); 

    // this is where we can change betweem using a smaller delta or not on the top level of recursion
    int64_t s;
    if (equalDelta) { // equal delta on all levels
        s = logN0 / (2*delta*delta);
    }
    else {  // small delta on top
        s = logN0*logN0;
    }   

    int64_t middle = groupedPartitionRecursive(A, N0, pivotVal, s, logB, delta);
    return middle;
}

