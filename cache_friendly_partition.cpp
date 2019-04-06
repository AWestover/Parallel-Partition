
// this is the version of the code where I recurse on the same program.
// parallel partition program

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

#define equalDelta false

// this is not changed, it is allways the same
int64_t computeS(int64_t n, double delta){
	return (int64_t)(log2(n) / (2*delta*delta));
}

// this is for when the input size gets small enough that making groups is a bad idea
int64_t serialPartitioner(int64_t* A, int64_t n, int64_t pivotVal) {
	int64_t low = 0; int64_t high = n-1;
	while(low < high){
		while(A[low] <= pivotVal && low < high) {
			low++;
		}
		while (A[high] > pivotVal && low < high){
			high--;
		}
		// swap
		int64_t tmp = A[low];
		A[low] = A[high];
		A[high] = tmp;
	}
	if(A[low] <= pivotVal){
		low++;
	}
	return low;
}

// this function is the most important part of this file
// it implements the grouped partition, and will be called from a different function that can implement special behavior on the top layer
int64_t groupedPartitionRecursive(int64_t* A, int64_t n, int64_t pivotVal, int64_t s, int64_t logB, double delta){
	int64_t b = 1<<logB;
	int64_t numGroups = n / (b*s);
	if (numGroups > 2){
		// first solve the top layer
		
		int64_t* X = (int64_t*)malloc(sizeof(int64_t)*s);
		// generate X (one list to store them all)
		for (int64_t i = 0; i < s; ++i) {
			X[i] = rand() % numGroups;
		}

		// serial partition on each group done in parallel across the groups
		int64_t* allVs = (int64_t*)malloc(sizeof(int64_t)*numGroups);

		// everything with idx < vmin has A[idx] <= pivotval
		// everything with idx >= vmax has A[idx] > pivotVal
		// so the thing to recurse on is A+vmin, with size vmax-vmin
		parallel_for (int64_t i = 0; i < numGroups; ++i) {

			int64_t low = 0;
			int64_t lowPidx = 0; // index into P
			int64_t lowXidx = 0; // index into X
			int64_t lowXoffset = (X[lowXidx] + i >= numGroups) ? X[lowXidx]+i - numGroups : X[lowXidx]+i; // cheaper than modulus

			int64_t high = (s<<logB)-1;
			int64_t highPidx = b-1; // index into P
			int64_t highXidx = s-1; // index into X
			int64_t highXoffset = (X[highXidx]+i >= numGroups) ? X[highXidx]+i - numGroups : X[highXidx]+i; // cheaper than modulus
			
			int64_t lowBase = (lowXoffset+numGroups*lowXidx)<<logB;
			int64_t highBase = (highXoffset+numGroups*highXidx)<<logB;

			while(low < high){
				while(A[lowPidx + lowBase] <= pivotVal && low < high) {
					low++;
					if(lowPidx != b-1){
						lowPidx += 1;
					} 
					else {
						lowPidx = 0;
						lowXidx += 1;
						lowXoffset = (X[lowXidx]+i >= numGroups) ? X[lowXidx]+i - numGroups : X[lowXidx]+i; // cheaper than modulus 
						lowBase =  (lowXoffset+numGroups*lowXidx) << logB;
					}
				}
				while (A[highPidx + highBase] > pivotVal && low < high){
					high--;
					if(highPidx != 0){
						highPidx -= 1;
					} 
					else {
						highPidx = b-1;
						highXidx -= 1;
						highXoffset = (X[highXidx]+i >= numGroups) ? X[highXidx]+i - numGroups : X[highXidx]+i; // cheaper than modulus
						highBase = (highXoffset+numGroups*highXidx) << logB;
					}
				}
				int64_t tmp = A[lowPidx + lowBase];
				A[lowPidx + lowBase] = A[highPidx + highBase];
				A[highPidx + highBase] = tmp;
			}
			if(A[lowPidx + lowBase] <= pivotVal){
				low++;
				if(lowPidx != b-1){
					lowPidx += 1;
				} else {
					lowPidx = 0;
					lowXidx += 1;
					lowXoffset = (X[lowXidx]+i >= numGroups) ? X[lowXidx]+i - numGroups : X[lowXidx]+i; // cheaper than modulus
				}
			}

			allVs[i] = lowPidx + ((lowXoffset+numGroups*lowXidx)<<logB);
		}

		// find the minimum in serial (we had data races when we tried to in parallel have everything write to the same variable at the same time (concurrent writing))
		int64_t vmin = n-1; int64_t vmax = 0;
		for (int64_t i = 0; i < numGroups; i++) {
			if(allVs[i] < vmin)
				vmin = allVs[i];
			if(allVs[i] > vmax)
				vmax = allVs[i];
		}
		

		// here is a bit of edge case cleanup
		// need to partition A from (s*b)*numGroups (ie sb(n/(sb))) to n

		int64_t leftOverStart = s*b*numGroups;
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
		else { // llllllllll ?????????? hhhh ???????? swap hs with the back of the extra section
			for(int64_t i = 0; i < successorPartitionSize; i++){
				int64_t tmp = A[n - 1 - i];
				A[n - 1 - i] = A[vmax];
				A[vmax] = tmp; 
				vmax += 1;
			}
			vmax = n - successorPartitionSize; 		
			assert (vmax < n); // this should never happen. It would indicate that the vmax code got messed up if it did. It would also indicate taht there are no successsors in the array. O wait, that is possible, dang, add a test case for this (adverserial pivotValue)
		}

		// solve the smaller subproblem, which starts at vmin, and has size vmax - vmin
		free(X);
		int64_t middle = vmin + groupedPartitionRecursive(A+vmin, vmax-vmin, pivotVal, computeS(vmax-vmin, delta), logB, delta);

		return middle;
	}
	else { // if the input is really small, delegate to a serial partitioner
		return serialPartitioner(A, n, pivotVal);	
	}
}

int64_t groupedPartition(int64_t* A, int64_t N0, int64_t pivotVal){
	double delta = 0.5;
	int64_t logB = 4;

	int64_t logN0 = (int64_t)log2(N0); 

	// this is where we can change betweem using a smaller delta or not on the top level of recursion
	int64_t s;
	if (equalDelta) { // equal delta on all levels
		s = logN0 / (2*delta*delta);
	}
	else { 	// small delta on top
		s = logN0*logN0;
	}	

	int64_t middle = groupedPartitionRecursive(A, N0, pivotVal, s, logB, delta);
	return middle;
}

/*
void checkCorrectness(){
	int64_t N0 = 1<<15;
	int64_t* A = (int64_t*)malloc(N0*sizeof(int64_t));
	int64_t countZeros = 0;
	for (int64_t i = 0; i < N0; ++i) {
		A[i] = 100*(rand()%2);
		if(A[i] == 0){
			countZeros += 1;
		}
	}

	int64_t pivotVal = 50;
	// solve the problem
	int64_t middle = groupedPartition(A, N0, pivotVal);
	
	assert (middle == countZeros);
	
	for (int64_t i = 0; i < middle; ++i) {
		assert(A[i] <= pivotVal);
	}
	for(int64_t i = middle; i < N0; ++i){
		assert(A[i] > pivotVal);
	}
	free(A);
}
*/
/*
int main() {
	srand(time(NULL)); 
	int64_t logN0 = 28;
	int64_t N0 = 1<<logN0;
	int64_t* A = (int64_t*)malloc(N0*sizeof(int64_t));
	for (int64_t i = 0; i < N0; ++i) {
		A[i] = 100*(rand()%2);
	}
	// A[i] <= pivotVal goes on one side of the partition, A[i] > pivotVal goes on the other side
	int64_t pivotVal = 50;
	assert(((int64_t)A)%64 == 0);

	clock_t t0 = clock();
	// solve the problem
	groupedPartition(A, N0, pivotVal);
	double timeTaken = (clock()-t0) / (double) CLOCKS_PER_SEC;
	checkCorrectness();
	cout << "time taken: " <<  timeTaken << endl;

	free(A);
	return 0;
}
*/
