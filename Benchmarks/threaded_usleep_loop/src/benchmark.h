#ifndef LOOPS_BENCHMARK_H
#define LOOPS_BENCHMARK_H

enum BENCHMARK {
  NOP = 0,
};

void run_benchmark(int threads, enum BENCHMARK benchmark);


#endif
