/*
 * Test Cases 5 - Stress time
 * CPS 310 (OS) Lab 1 - Heap Manager
 * Author: Nisarg Raval
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "dmm.h"

// PFL prints the free list if DBFLAG is set
//#define DBFLAG

#ifdef DBFLAG
#define PFL print_freelist()
#else
#define PFL
#endif

#ifdef HAVE_DRAND48
#define RAND() (drand48())
#define SEED(x) (srand48((x)))
#else
#define RAND() ((double)random()/RAND_MAX)
#define SEED(x) (srandom((x)))
#endif

#define PSEDUO_RND_SEED 0 // 1 for non-deterministic seeding
#define BUFLEN 1000
#define LOOPCNT 50000
#define ALLOC_SIZE MAX_HEAP_SIZE/100
#define ALLOC_CONST 0.6

//Test Case 5: allocate and free in random order, remove integrity check and print statement for timing
// return 0 if fails and 1 if passes
int test_case5(double *acc, int *acc_abs) {
	int size;
	int itr;
	void *ptr[BUFLEN];
	int i, j;
	double randvar;
	int fail = 0;
	int max_nb = 0;
	int nalloc = 0;

	for (i = 0; i < BUFLEN; i++) {
		ptr[i] = NULL;
	}

	/* Set the PSEUDO_RANDOM_SEED for pseduo random seed initialization based on time, i.e.,
	 * the random values changes after each execution 
	 */
	if (PSEDUO_RND_SEED)
		SEED(time(NULL));

	assert(
			MAX_HEAP_SIZE >= 1024 * 1024
					&& "MAX_HEAP_SIZE is too low; Recommended setting is at least 1MB for TC5 (test_stress)");

	for (i = 0; i < LOOPCNT; i++) {
		itr = (int) (RAND() * BUFLEN); //randomly choose an index for alloc/free

		randvar = RAND(); //flip a coin to decide alloc/free

		if (randvar < ALLOC_CONST && ptr[itr] == NULL) { //if the index is not already allocated allocate random size memory
			size = (int) (RAND() * ALLOC_SIZE);			
			if (size > 0) {
				ptr[itr] = dmalloc(size);
				nalloc++;
			} else
				continue;
			if (ptr[itr] == NULL) {
				fail++;
				continue;
			}
	
			PFL;

		} else if (randvar >= ALLOC_CONST && ptr[itr] != NULL) { //free memory
			dfree(ptr[itr]);
			ptr[itr] = NULL;
		}
	}

	//free all memory
	for (i = 0; i < BUFLEN; i++) {
		if (ptr[i] != NULL) {
			dfree(ptr[i]);
			ptr[i] = NULL;
		}
	}

	//*acc = (double) (nalloc-fail)/ (double) nalloc; //acc shows the percentage of successful dmalloc
	*acc = (double) (LOOPCNT-fail)/ (double) LOOPCNT; //acc shows the percentage of successful dmalloc/free
	*acc_abs = LOOPCNT-fail;
	return 1;
}

int main(int argc, char** argv) {

	clock_t begin, end;

	double nbytes = 0.0; //successful bytes (%) allocation in a test case
	double time = 0.0; //execution time of a test case
	double acc = 0.0; //percentage of correct alloc/free
	int nbytes_abs = 0; //absolute value of nbytes
	int acc_abs = 0;  // absolute value of acc
	
	//Test Case 5
	nbytes = 0.0;
	acc = 0.0;
	begin = clock();
	int rc = test_case5(&acc,&acc_abs);
	end = clock();
	time = (double) (end - begin) / CLOCKS_PER_SEC;
	if(rc == 1)
		fprintf(stderr, "TC5 passed!\n");
	//Format: TC5: Success ExecTime Accuracy AccuracyABS
	fprintf(stderr,"TC5: ExecTime Accuracy  AccuracyABS\n");
	fprintf(stderr," %f  %f %d\n",time,acc,acc_abs);
	fflush(stderr);

	return 0;
}
