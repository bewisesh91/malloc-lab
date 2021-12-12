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
#define WSIZE 4             // Word and header/footer size (bytes)
#define DSIZE 8             // Double word size (bytes)
#define CHUNKSIZE (1 << 12) // Extend heap by this amount (bytes)
#define MINIMUM   24        // Minimum Size (bytes)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* freeList의 이전 포인터와 다음 포인터 계산 */
#define NEXT_FLP(bp)  (*((char**)(bp) + 1))      // 다음 free list 요소의 bp를 가져옴
#define PREV_FLP(bp)  (*((char**)(bp)))          // 이전 free list 요소의 bp를 가져옴

/* glbal variable */
static void *heap_listp; // 정적 전역변수를 사용한 할당기
static void *extend_heap(size_t);
static void *coalesce(void *);
static void *find_fit(size_t);
static void place(void *, size_t);
static void add_free(void *);
static void del_free(void *);
static void *free_listp; // free block들의 시작점을 나타내는 포인터

/* mm_init - initialize the malloc package. */
int mm_init(void){
    /* Create the initial empty heap */   
    if ((heap_listp = mem_sbrk(8 * WSIZE)) == (void *) - 1)
        return -1;
    PUT(heap_listp, 0);  
    PUT(heap_listp + (1 * WSIZE), PACK(MINIMUM, 1));
    PUT(heap_listp + (2 * WSIZE), NULL);
    PUT(heap_listp + (3 * WSIZE), NULL);
    PUT(heap_listp + (6 * WSIZE), PACK(MINIMUM, 1));
    PUT(heap_listp + (7 * WSIZE), PACK(0, 1));

    free_listp = heap_listp + (2 * WSIZE);
    
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if (size < MINIMUM) {
        size = MINIMUM;
    }
    
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/* mm_malloc - Allocate a block by incrementing the brk pointer. Always allocate a block whose size is a multiple of the alignment. */
void *mm_malloc(size_t size){
    size_t asize;
    size_t extendsize;
    char* bp;

    if (size == 0) {
        return NULL;
    }
    
    if (size <= MINIMUM - DSIZE) {
        asize = MINIMUM;
    } 
    else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void add_free(void *bp){
    NEXT_FLP(bp) = free_listp;
    PREV_FLP(bp) = NULL;
    PREV_FLP(free_listp) = bp;
    free_listp = bp;
}

static void del_free(void *bp){
    if (bp == free_listp) {
        PREV_FLP(NEXT_FLP(bp)) = PREV_FLP(bp);
        free_listp = NEXT_FLP(bp);
        return;
    }
    NEXT_FLP(PREV_FLP(bp)) = NEXT_FLP(bp);
    PREV_FLP(NEXT_FLP(bp)) = PREV_FLP(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        del_free(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        del_free(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else if (!prev_alloc && !next_alloc) {
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))));
        del_free(PREV_BLKP(bp));
        del_free(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    add_free(bp);
    return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE((char *)oldptr - WSIZE) - DSIZE; // header의 사이즈
    if (size < copySize) {
      copySize = size;
    }
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void* find_fit(size_t asize) 
{
    void *bp = free_listp;
    while (!GET_ALLOC(HDRP(bp))) {
        if (asize <= GET_SIZE(HDRP(bp))) {
            return bp;
        }
        bp = NEXT_FLP(bp);
    }
    return NULL;
}

static void place(void *bp, size_t asize) 
{
    size_t csize = GET_SIZE(HDRP(bp));
    del_free(bp);
    if (csize - asize >= MINIMUM) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

