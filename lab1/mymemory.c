#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>

#define SYSTEM_MALLOC 0

#define DEBUG 0
// High level indicates more important message
#define LEVEL 10

#define debug_print(lvl, fmt, ...) \
            do { if (DEBUG && lvl >= LEVEL) { \
      fprintf(stdout, ">>> "); \
      fprintf(stdout, fmt, __VA_ARGS__); \
  } } while (0)

/*

Model

 ------------------------
| Header struct          |
|------------------------|
| Data                   |
|------------------------|
|                        |
           ...
|                        |
|------------------------|


*/
// Number of increments per sbrk call
#define SBRK_INCREMENT 4096

struct Header {
  struct Header* next; // Address of the next block
  unsigned int size; // 0 if this block is free, size of the block if
         // this block has been allocated
};

// a global lock for synchronization
pthread_mutex_t globalLock = PTHREAD_MUTEX_INITIALIZER;

// Find out the size of a pointer
// This will be used to calculate padding
const int p_size = sizeof(char *);

// The start heap in test_malloc.c
extern void *start_heap;

// We're using char pointers because a char is always 1 byte
// so pointer arithmetic will work the way we want
char* lastBlock = NULL;  // Address of the last block
char* limit = NULL; // Address of the program break

/* For testing purposes only */
// Resets the heap (by moving a few pointers and zeroing everything) so we
// can start over
/*
void reset() {
  // zero everything
  char* start;
  for(start = start_heap; start < limit; start++) {
    *start = 0;
  }
  lastBlock = start_heap;
}*/
/* End test section */

/* mymalloc: allocates memory on the heap of the requested size. The block
             of memory returned should always be padded so that it begins
             and ends on a word boundary.
     unsigned int size: the number of bytes to allocate.
     retval: a pointer to the block of memory allocated or NULL if the
             memory could not be allocated.
             (NOTE: the system also sets errno, but we are not the system,
                    so you are not required to do so.)
*/
void *mymalloc(unsigned int size) {
  // No need to lock the first section of code since it's not using any
  // shared variables (except start_heap which never changes).

  /* BEGIN NO LOCK NEEDED */
  #if SYSTEM_MALLOC
  debug_print(10, "%s\n", "Using system malloc");
  return malloc(size);
  #endif

  if (size == 0) {
    fprintf(stderr, "%s\n", "Size 0 passed to mymalloc");
    return NULL;
  }

  // Calculate how many bytes to allocate
  unsigned int total_size = sizeof(struct Header) // Header
      + size; // Requested size

  // Calculate the padding
  unsigned int padding = 0;
  if (total_size % p_size != 0) {
    padding = p_size - (total_size % p_size);
  }
  total_size += padding;
  debug_print(5, "Requested size: %d, Padding: %d, Total size: %d\n", size, padding, total_size);

  // Look for a free block using best fit
  struct Header* currentBlock = (struct Header*) start_heap;
  char* bestBlock = NULL;
  unsigned int bestDiff = INT_MAX;

  /* END NO LOCK NEEDED */

  pthread_mutex_lock(&globalLock);
  while(currentBlock < (struct Header *) lastBlock) {
    if (currentBlock->size == 0) {
      // We found a free block, if there are free blocks
      // after this one, merge them together (coalescing)

      // Find the next block that is allocated
      struct Header* nextBlock = currentBlock->next;
      while(nextBlock != NULL && nextBlock->size == 0) {
        nextBlock = nextBlock->next;
      }

      currentBlock->next = nextBlock;

      if (nextBlock == NULL) {
        // nextBlock is NULL means we merged currentBlock with
        // the last block, so repoint the lastBlock pointer
        // to the current block
        lastBlock = (char *) currentBlock;

        // We searched through everything, no need
        // to continue
        break;
      }

      // Find a free block such that the difference between
      // the free space and the required space is minimal
      int diff = ((char *) currentBlock->next) - ((char *) currentBlock) - total_size;
      debug_print(5, "Diff: %d\n", diff);
      if (diff >= 0 && diff < bestDiff) {
        bestBlock = (char *) currentBlock;
        bestDiff = diff;
      }
    }
    currentBlock = currentBlock->next;
  }

  // Address of the next block
  char* nextBlock;
  struct Header* nextHdr;

  if (bestBlock == NULL) {
    // We did not find any block that will fit
    // so allocate in the last block
    debug_print(5, "Allocating in last block: %p\n", lastBlock);

    if (limit == NULL) {
      // This will only happen once, the first time mymalloc is called
      lastBlock = start_heap;
      limit = start_heap;
      debug_print(10, "Initial program break %p\n", lastBlock);
    }

    bestBlock = lastBlock;
    nextBlock = lastBlock + total_size;
    nextHdr = (struct Header*) nextBlock;

    // Difference in address (in bytes) between nextBlock
    // and limit (need also the size of the struct Header for
    // the nextBlock's header)
    int diff = nextBlock + sizeof(struct Header) - limit;

    // Positive difference means we need more space
    if (diff > 0) {
      // Increment by the smallest multiple of SBRK_INCREMENT
      // that is larger than diff
      int increment = (diff / SBRK_INCREMENT + 1) * SBRK_INCREMENT;

      if (sbrk(increment) == (void *) -1) {
        debug_print(10, "sbrk could not increment: %d bytes\n", increment);
        fprintf(stderr, "sbrk failed, mymalloc cannot continue\n");

        pthread_mutex_unlock(&globalLock);
        return NULL;
      }
      limit += increment;
      debug_print(10, "Incremented by %d, New program break: %p\n", increment, limit);
    }

    lastBlock = nextBlock;

    // Mark the last block as free
    nextHdr->next = NULL;
    nextHdr->size = 0;
  } else {
    // Otherwise we're putting the block in a free block (which is in between two
    // allocated blocks)
    debug_print(10, "Allocating between blocks, Diff: %d\n", bestDiff);

    // Need to decide if we need to split the free block into
    // two: the first block is being allocated and the second
    // block being free. We should only split the free block
    // if there is enough room for a Header and at least
    // one pointer in the second block.
    if (bestDiff >= sizeof(struct Header) + p_size) {
      // Create the second block, which is free
      nextBlock = bestBlock + total_size;
      nextHdr = (struct Header*) nextBlock;

      nextHdr->next = ((struct Header*) bestBlock)->next;
      nextHdr->size = 0;
      debug_print(10, "Split free block in two: head %p, next: %p\n",
      nextHdr, nextHdr->next);

    } else {
      // Either it's a perfect fit, or is some space leftover
      // but not enough for a header and a pointer.

      // So we have to add this difference as even more
      // "padding" to make our pointer arithmetics right
      total_size += bestDiff;
      nextBlock = bestBlock + total_size;
      nextHdr = (struct Header*) nextBlock;

      debug_print(10, "%s\n", "Not enough room for another free block");
    }
  }

  // Store the address of the next block in the header
  // and record the size of this block
  struct Header *currentHdr = ((struct Header*) bestBlock);
  currentHdr->next = nextHdr;
  currentHdr->size = total_size;

  // The address to be returned to the user
  char * returnVal = bestBlock + sizeof(struct Header);
  debug_print(10, "Head: %p, Return: %p, Next: %p\n", bestBlock,
  returnVal, nextBlock);

  pthread_mutex_unlock(&globalLock);
  return returnVal;
}

/* myfree: unallocates memory that has been allocated with mymalloc.
     void *ptr: pointer to the first byte of a block of memory allocated by
                mymalloc.
     retval: 0 if the memory was successfully freed and 1 otherwise.
             (NOTE: the system version of free returns no error.)
*/
unsigned int myfree(void *ptr) {
  #if SYSTEM_MALLOC
  debug_print(10, "%s\n", "Using system free");
  free(ptr);
  return 0;
  #endif

  if (ptr == NULL) {
    fprintf(stderr, "NULL pointer passed to myfree\n");
    return 1;
  }

  debug_print(5, "Asked to free: %p\n", ptr);
  char* target = ((char *) ptr) - sizeof(struct Header);
  struct Header* targetHdr = (struct Header*) target;

  // Check if the address is between the start address and our last
  // block (otherwise we may get a SEGFAULT if we try to access its
  // fields). Check to see if difference in address corresponds to
  // the size (for error checking).
  pthread_mutex_lock(&globalLock);
  if (target >= (char *) start_heap && target <= lastBlock &&
    ((char *) targetHdr->next) - target == targetHdr->size) {

    targetHdr->size = 0;

    debug_print(10, "Free successful, Start: %p, End: %p, Size: %d\n", target, targetHdr->next, targetHdr->size);
    // No coalescing here, it is done in mymalloc
    pthread_mutex_unlock(&globalLock);
    return 0;
  }

  fprintf(stderr, "Invalid pointer given to myfree: %p\n", ptr);
  pthread_mutex_unlock(&globalLock);
  return 1;
}
