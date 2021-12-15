/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "jungle3rd_week06",
    /* First member's full name */
    "group_09",
    /* First member's email address */
    "bewise.seunghyun@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4             /* word and header, footer size */
#define DSIZE 8             /* double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extended heap by this amount */
#define LISTLIMIT 20        /* free list max num */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at adress p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* get free list's SUCC and PREC pointer */
#define PRED_FREE(bp) (*(char **)(bp))
#define SUCC_FREE(bp) (*(char **)(bp + WSIZE))

static void *heap_listp;
static void *segregation_list[LISTLIMIT];

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void remove_block(void *bp);
static void insert_block(void *bp, size_t size);

/* mm_init - initialize the malloc package. */
int mm_init(void){
    int list;

    for (list = 0; list < LISTLIMIT; list++){
        segregation_list[list] = NULL;
    }

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp = heap_listp + 2 * WSIZE;

    /* Extended the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL){
        return -1;
    }
    return 0;
}

static void remove_block(void *bp){
    int list = 0;
    size_t size = GET_SIZE(HDRP(bp));

    while ((list < LISTLIMIT - 1) && (size > 1)){
        size >>= 1;
        list++;
    }

    if (SUCC_FREE(bp) != NULL){
        if (PRED_FREE(bp) != NULL){
            PRED_FREE(SUCC_FREE(bp)) = PRED_FREE(bp);
            SUCC_FREE(PRED_FREE(bp)) = SUCC_FREE(bp);
        }
        else{
            PRED_FREE(SUCC_FREE(bp)) = NULL;
            segregation_list[list] = SUCC_FREE(bp);
        }
    }
    else{
        if (PRED_FREE(bp) != NULL){
            SUCC_FREE(PRED_FREE(bp)) = NULL;
        }
        else{
            segregation_list[list] = NULL;
        }
    }
    return;
}

static void insert_block(void *bp, size_t size){
    int list = 0;
    void *search_ptr;
    void *insert_ptr = NULL;

    while ((list < LISTLIMIT - 1) && (size > 1)){
        size >>= 1;
        list++;
    }

    search_ptr = segregation_list[list];
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))){
        insert_ptr = search_ptr;
        search_ptr = SUCC_FREE(search_ptr);
    }

    if (search_ptr != NULL){
        if (insert_ptr != NULL){
            SUCC_FREE(bp) = search_ptr;
            PRED_FREE(bp) = insert_ptr;
            PRED_FREE(search_ptr) = bp;
            SUCC_FREE(insert_ptr) = bp;
        }
        else{
            SUCC_FREE(bp) = search_ptr;
            PRED_FREE(bp) = NULL;
            PRED_FREE(search_ptr) = bp;
            segregation_list[list] = bp;
        }
    }
    else{
        if (insert_ptr != NULL){
            SUCC_FREE(bp) = NULL;
            PRED_FREE(bp) = insert_ptr;
            SUCC_FREE(insert_ptr) = bp;
        }
        else{
            SUCC_FREE(bp) = NULL;
            PRED_FREE(bp) = NULL;
            segregation_list[list] = bp;
        }
    }
    return;
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1 : 맨 아래 조건으로 충족*/

    /* Case 2 */
    if (prev_alloc && !next_alloc){ 
        remove_block(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    
    /* Case 3 */
    else if (!prev_alloc && next_alloc){ 
        remove_block(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* Case 4 */
    else if (!prev_alloc && !next_alloc){
        remove_block(PREV_BLKP(bp));
        remove_block(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_block(bp, size);
    return bp;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void *find_fit(size_t asize){
    /* First-fit search */
    void *bp;
    int list = 0;
    size_t searchsize = asize;

    while (list < LISTLIMIT){
        searchsize >>= 1;
        list++;
        if ((list == LISTLIMIT - 1) || (searchsize <= 1) && (segregation_list[list] != NULL)){
            bp = segregation_list[list];

            while ((bp != NULL) && (asize > GET_SIZE(HDRP(bp)))){
                bp = SUCC_FREE(bp);
            }
            if (bp != NULL){
                return bp;
            }
        }
    }
    return NULL; /* no fit */
}


static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    remove_block(bp);
    if ((csize - asize) >= (2 * DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);
    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* mm_malloc - Allocate a block by incrementing the brk pointer. Always allocate a block whose size is a multiple of the alignment. */
void *mm_malloc(size_t size){
    int asize = ALIGN(size + SIZE_T_SIZE);

    // size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/* mm_free - Freeing a block does nothing. */
void mm_free(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

/* mm_realloc - Implemented simply in terms of mm_malloc and mm_free */
void *mm_realloc(void *ptr, size_t size){
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}




