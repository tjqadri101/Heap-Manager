//Talal Javed Qadri
//September 11, 2014
//  *********** Heap Manager ************

// ***** Section 1: Pre-processor Directives Section *****

#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/* I used the boundary tags idea from the Bryant
 * and OHallaron book (chapter 9).
 * I did not use the metadata_t structure
 * My blocks have a header and a footer (8 bytes each)
 * I used an implicit free list
 * Minimum block size of free block is 24 bytes (8 bytes for payload, 16 bytes for headers and footers
 * Possible minimum block size of allocated block is 24 bytes (16 bytes for header and footer, 8 bytes for payload for longword allignment)
 * However, minimum block size for allocated blocks is also kept at 32 bytes to match with free blocks
 */

//The macros below are adapted from Chapter 9 of Bryant and OHallaron book (chapter 9).

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address pointer */
#define GET(pointer) (*(size_t *)(pointer))
#define PUT(pointer, value) (*(size_t *)(pointer) = (value))

/* Read the size and allocated fields from address pointer */
#define GET_SIZE(pointer) (GET(pointer) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WORD_SIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 2*WORD_SIZE) //WORD_SIZE is equal to size of long word which is 8 bytes

/* Given block ptr bp, compute address of next and previous blocks in heap (not just free list)*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WORD_SIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - 2*WORD_SIZE)))

typedef struct metadata {
       /* size_t is the return type of the sizeof operator. Since the size of
 	* an object depends on the architecture and its implementation, size_t 
	* is used to represent the maximum size of any object in the particular
 	* implementation. 
	* size contains the size of the data object or the amount of free
 	* bytes 
	*/
	size_t size;
	struct metadata* next;
	struct metadata* prev; //What's the use of prev pointer?
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

//static metadata_t* freelist = NULL;//not used;
static void* heapP = NULL; //points to the prologue block of the heap
static void* freeList = NULL; //points to head of free list which is implicit so points to start of list always
static void* epilogue = NULL;


//*****Section 2: Function Prototypes*****
//Define private function prototypes

/*Perform a first-fit search of the explicit free list for a block that satisfies a size of asize bytes*/
static void *first_fit(size_t asize);

/*place the requested block at the beginning of the free block, splitting only if the size of the remainder would equal or exceed the minimum block size*/
static void place(void *bp, size_t asize);

/*Combine the adjacent blocks into a single contiguous heap block*/
static void *coalesce(void *bp);

//*****Section 3: Subroutines*****

void* dmalloc(size_t numbytes) {
	size_t asize; /* Adjusted block size */
	char *bp;//pointer to be returned to user, point to location just after the ending location of block header
	//printf("debugging\n");
	if(freeList == NULL) { 			//Initialize through sbrk call first time
		if(!dmalloc_init())
			return NULL;
	}

	assert(numbytes > 0);

	
	 asize = ALIGN(numbytes) + 2*WORD_SIZE; //alignment gives the payload size with the required padding 
							//an addition of 2*WORD_SIZE of memory is provided for the head and footer

	 /* Search the free list for a fit using the first_fit allocation strategy*/
	//printf("debugging\n");
	 if ((bp = first_fit(asize)) != NULL) {
		//printf("debugging\n");
	 	place(bp, asize);
		//printf("debugging\n");
	 	return bp;
	 }

	return NULL;
}

void dfree(void* ptr) {
	if(ptr != NULL){
		/* Your free and coalescing code goes here */
		size_t size = GET_SIZE(HDRP(ptr));
		
		PUT(HDRP(ptr), PACK(size, 0));//set the allocation bit to signify that the block is free
		PUT(FTRP(ptr), PACK(size, 0));
		coalesce(ptr);
	}
	else
		printf("Null pointer. Can't be freed\n");
}

bool dmalloc_init() {

	/* I initialize freelist pointers to NULL and also used prologue and epilogue blocks to avoid errors at edge-cases of heap
 	*/
	size_t freeListSize;
	size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
	heapP = (void*) sbrk(max_bytes); // returns heap_region, which is initialized to freeList
	//check if memory allocated
	if (heapP == (void *)-1)
		return false;
	PUT(heapP, 0); /* Alignment padding */
	//printf("debugging\n");
	PUT(heapP + (1*WORD_SIZE), PACK(WORD_SIZE,1)); /* Prologue header */
	//printf("debugging\n");
	PUT(heapP + (2*WORD_SIZE), PACK(WORD_SIZE, 1)); /* Prologue footer */
	//printf("debugging\n");
	freeListSize = max_bytes - 4*WORD_SIZE;// 4 long words (8 bytes each): 1 for initial alignment padding, 2 for prologue block, 1 for epilogue header
	//printf("debugging\n");	
	freeList = heapP + (3*WORD_SIZE);//Initialize location for free list header
	//printf("debugging\n");
	PUT(freeList, PACK(freeListSize, 0));//Initialize free list header
	PUT(FTRP(freeList + WORD_SIZE), PACK(freeListSize, 0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(freeList + WORD_SIZE)), PACK(0, 1)); /* New epilogue header */
	epilogue = HDRP(NEXT_BLKP(freeList + WORD_SIZE));
	heapP += 2 * WORD_SIZE;//always point to Prologue footer from now on
	//printf("Heap pointer %p, Free List pointer after initialization %p\n", heapP, freeList);
	//printf("debugging\n");
	return true;
}

/*Perform a first-fit search of the explicit free list for a block that satisfies a size of asize bytes*/
void *first_fit(size_t asize){
	void *freeList_head = freeList;
	while(freeList_head != epilogue) {
		if((((size_t) (GET_SIZE(freeList_head))) >= asize) && !GET_ALLOC(freeList_head))
			return (freeList_head + WORD_SIZE);// pointer to location just below block header
		freeList_head = HDRP(NEXT_BLKP(freeList_head + WORD_SIZE));//move to next block in heap
	}
	return NULL;
}

/*Place the requested block at the beginning of the free block, splitting only if the size of the remainder would equal or exceed the minimum block size*/
void place(void *bp, size_t asize){
	size_t sizeDifference= ((size_t) (GET_SIZE(HDRP(bp)))) - asize;//always a multiple of 8 because of our alignment methodology 
	if(sizeDifference >= 3*WORD_SIZE){
		//Spllit to create an allocated block
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		//update free block
		PUT(HDRP(NEXT_BLKP(bp)), PACK(sizeDifference, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(sizeDifference, 0));
	}
	else{
		PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));//set the allocated bit to 1
		PUT(FTRP(bp), PACK(GET_SIZE(FTRP(bp)), 1));
		//whole block allocated
	}
}

void *coalesce(void *bp){
	void *prevHeapBPtr = PREV_BLKP(bp); 
	void *nextHeapBPtr = NEXT_BLKP(bp);
	size_t prev_alloc = GET_ALLOC(FTRP(prevHeapBPtr));
	size_t next_alloc = GET_ALLOC(HDRP(nextHeapBPtr));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { /*Both previous and next blocks in heap are allocated */
		return bp;
	}

	else if (prev_alloc && !next_alloc) { /* Next block in heap is free, coalesce with it*/
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
	}

	else if (!prev_alloc && next_alloc) { /* Previous block in heap is free so coalesce with it */
		size += GET_SIZE(HDRP(prevHeapBPtr));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(prevHeapBPtr), PACK(size, 0));
		bp = prevHeapBPtr;
	}

	else { /*Both previous and next blocks in heap are free so coalesce with them both */
		size += GET_SIZE(HDRP(prevHeapBPtr)) + GET_SIZE(HDRP(nextHeapBPtr));
		PUT(HDRP(prevHeapBPtr), PACK(size, 0));
		PUT(FTRP(nextHeapBPtr), PACK(size, 0));
		bp = prevHeapBPtr;
	}
	return bp;
}


/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
//The method below was provided by instructors
//It uses the metadata_t structure
//void print_freelist() {
//	metadata_t *freelist_head = freelist;
//	while(freelist_head != NULL) {
//		DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",freelist_head->size,freelist_head,freelist_head->prev,freelist_head->next);
//		freelist_head = freelist_head->next;
//	}
//	DEBUG("\n");
//}


//This method prints the free list for my implementation of the heap which involoves boundary tags
//Since I use an implicit free list I can potentially go through the entire heap to find a block
//Hence, the next and previos block pointers represented the heads of the next and previous blocks in the heap 
//These next or previous blocks may or may not be allocated
void print_freelist() {
	void *heapPrev, *heapNext;
	void *freeList_head = freeList;
	printf("Printing\n");
	while(freeList_head != epilogue) {
		heapPrev = HDRP(PREV_BLKP(freeList_head + WORD_SIZE));
		heapNext = HDRP(NEXT_BLKP(freeList_head + WORD_SIZE));
		if(!GET_ALLOC(freeList_head))
			DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",(size_t) (GET_SIZE(freeList_head)),freeList_head, heapPrev,heapNext);
		freeList_head = heapNext;

	}
	DEBUG("\n");
}
