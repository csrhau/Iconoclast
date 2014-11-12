#ifndef LOOPS_BENCHMARK_H
#define LOOPS_BENCHMARK_H

enum POLICY {
  ROBIN = 1,
  PACKED = 2,
};

enum BENCHMARK {
  NOP = 0,
};

void run_benchmark(int jobs, enum POLICY policy, enum BENCHMARK benchmark);


#endif
