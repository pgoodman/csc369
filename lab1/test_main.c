/* Copyright 2014 Peter Goodman, all rights reserved. */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "test_heap.h"
#include "test_trace.h"

// Mutex that can be used by course code.
pthread_mutex_t mywait;
pthread_t threads[MAX_NUM_THREADS];
struct TraceThread trace_threads[MAX_NUM_THREADS];

// Heap used by `mem_sbrk`.
static struct Heap *heap;

// Optional heap initialization function that teams can use.
extern int myinit(void) __attribute__((weak));

// Stub for the system `sbrk` that re-routes everything through `heap`.
void *mem_sbrk(intptr_t size) {
  return ShiftBreak(heap, size);
}

// Stub for mmap.
void *mem_mmap(void *x, ...) {
  printf("BAD: Program used `mmap` instead of `sbrk`.\n");
  errno = ENOMEM;
  (void) x;
  return (void *) -1;
}

// TODO(pag): Implement mmap?

// Simple configuration of the heap. No debugging performed.
static void ConfigSimple(struct HeapOptions *options) {
  options->use_reentrant_sbrk = 1;

  options->uninit_poison_val = 0x33;
  options->init_poison_val = 0xAA;
  options->malloc_poison_val = 0x77;
  options->free_poison_val = 0xDE;
}

// Parses the lines of the trace file.
static void ParseTraceFile(FILE *fp) {
  char line_buff[32] = {'\0'};
  int line = 1;
  while (NULL != fgets(line_buff, 31, fp)) {
    line_buff[31] = '\0';
    ParseTraceLine(line_buff, line++);
    line_buff[0] = '\0';
  }
}

// Loads the trace file.
static void LoadTraceFile(const char *file_name) {
  FILE *trace_file = NULL;
  if (NULL == (trace_file = fopen(file_name, "r"))) {
    fprintf(stderr, "Unable to open trace file '%s' for reading.\n", file_name);
    exit(EXIT_FAILURE);
  }
  ParseTraceFile(trace_file);
  fclose(trace_file);
}

// Runs an experiment.
int main(int argc, const char *argv[]) {
  int tid = 0;
  int num_threads = 0;

  if (2 != argc) {
    fprintf(stderr, "Usage: %s trace_file\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Setup.
  pthread_mutex_init(&mywait, NULL);
  LoadTraceFile(argv[1]);
  heap = AllocHeap(33546240 /* 32 MiB - 2 redzone pages */, ConfigSimple);
  if (myinit) myinit();

  num_threads = NumThreads();
  for (tid = 0; tid < num_threads; tid++) {
    trace_threads[tid].heap = heap;
    trace_threads[tid].id = tid;
    pthread_create(&(threads[tid]), NULL, ExecuteTraces, &(trace_threads[tid]));
  }

  for (tid = 0; tid < num_threads; tid++) {
    pthread_join(threads[tid], NULL);
  }

  Report(heap);

  // Teardown.
  FreeHeap(heap);
  return EXIT_SUCCESS;
}
