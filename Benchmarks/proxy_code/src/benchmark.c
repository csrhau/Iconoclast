#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "benchmark.h"

struct benchmark_args {
  int sleep_us;
  int loop_kcycles;
  int macro_cycles;
};

static void * proxy_benchmark(void * args) {
  struct benchmark_args * proxy_args = (struct benchmark_args *) args;
  int macro_cycle, loop_kcycle, micro_cycle;
  for (macro_cycle = 0; macro_cycle < proxy_args->macro_cycles; ++macro_cycle) {
    for (loop_kcycle = 0; loop_kcycle < proxy_args->loop_kcycles; ++loop_kcycle) {
      for (micro_cycle = 0; micro_cycle < 1000; ++micro_cycle) {
        asm volatile("nop");
      }
    }
    usleep(proxy_args->sleep_us);
  }
  return NULL;
}

void run_benchmark(int threads, int sleep_us, int loop_kcycles,
                   int macro_cycles, int verbose) {
  void ** retval = NULL;
  pthread_t thread_handles[threads];
  int thread;
  struct benchmark_args proxy_args = { sleep_us, loop_kcycles, macro_cycles };
  for (thread = 0; thread < threads; ++thread) {
    pthread_create(&thread_handles[thread], NULL,
                    &proxy_benchmark,
                   (void *) &proxy_args);
  }
  for (thread = 0; thread < threads; ++thread) {
    pthread_join(thread_handles[thread], retval);
  }
}
