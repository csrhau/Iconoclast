#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "benchmark.h"

static void * nop_benchmark_1(void * args) {
  printf("benchmark 1\n");
  asm(".intel_syntax noprefix");
  asm volatile("top1:    ; \
                jmp top1 ");
  asm(".att_syntax prefix");
  return NULL;
}

static void * nop_benchmark_2(void * args) {
  printf("benchmark 2\n");
  asm(".intel_syntax noprefix");
  asm volatile("top2:    ; \
                nop     ; \
                jmp top2 ");
  asm(".att_syntax prefix");
  return NULL;
}

static void * nop_benchmark_3(void * args) {
  printf("benchmark 3\n");
  asm(".intel_syntax noprefix");
  asm volatile("top3:    ; \
                nop     ; \
                nop     ; \
                jmp top3 ");
  asm(".att_syntax prefix");
  return NULL;
}

static void * nop_benchmark_4(void * args) {
  printf("benchmark 4\n");
  asm(".intel_syntax noprefix");
  asm volatile("top4:    ; \
                nop     ; \
                nop     ; \
                nop     ; \
                jmp top4 ");
  asm(".att_syntax prefix");
  return NULL;
}

void run_benchmark(int jobs, enum POLICY policy, enum BENCHMARK benchmark) {
  int threads = 0;
  if (policy == ROBIN) {
    threads = jobs < 4 ? jobs : 4;
  } else if (policy == PACKED) {
    threads = (int) 1 + (jobs-1) / 4;
  }
  printf("Spinning up %d threads for %d jobs \n", threads, jobs);

  int thread;
  pthread_t thread_handles[jobs];
  void * (*benchmark_function)(void *);
  void ** retval = NULL;


  for (thread = 0; thread < threads; ++thread) {

    int my_jobs = 0;
    if (policy == ROBIN) {
      if (jobs % threads != 0) {
        fprintf(stderr, "Uneven distribution not supported\n");
        exit(EXIT_FAILURE);
      }
      my_jobs = jobs / threads;
    } else if (policy == PACKED) {
        int remaining = jobs - (thread) * 4;
        my_jobs = remaining < 4 ? remaining : 4;
    } else {
      fprintf(stderr, "Invalid benchmark! exiting");
      exit(EXIT_FAILURE);
    }
    printf("My jobs is %d\n", my_jobs);
    switch (my_jobs) {
      case 1: benchmark_function = &nop_benchmark_1; break; 
      case 2: benchmark_function = &nop_benchmark_2; break; 
      case 3: benchmark_function = &nop_benchmark_3; break; 
      case 4: benchmark_function = &nop_benchmark_4; break; 
      default: printf("FUCK!\n"); exit(EXIT_FAILURE);
    }
    pthread_create(&thread_handles[thread], NULL,
                 benchmark_function,
                 (void *) NULL);
  }
  for (thread = 0; thread < jobs; ++thread) {
    pthread_join(thread_handles[thread], retval);
  }
}
