#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "benchmark.h"

static void * nop_benchmark(void * args) {
  asm(".intel_syntax noprefix");
  asm volatile("top:    ; \
                nop     ; \
                nop     ; \
                nop     ; \
                jmp top ");
  asm(".att_syntax prefix");
  return NULL;
}

void run_benchmark(int threads, enum BENCHMARK benchmark) {
  int thread;
  pthread_t thread_handles[threads];
  void * (*benchmark_function)(void *);
  void ** retval = NULL;

  switch (benchmark) {
    case NOP:
      benchmark_function = &nop_benchmark; 
      break;
    default:
      fprintf(stderr, "Invalid benchmark! exiting");
      exit(EXIT_FAILURE);
      break;
  }

  for (thread = 0; thread < threads; ++thread) {
    pthread_create(&thread_handles[thread], NULL,
                   benchmark_function,
                   (void *) NULL);
  }
  for (thread = 0; thread < threads; ++thread) {
    pthread_join(thread_handles[thread], retval);
  }
}
