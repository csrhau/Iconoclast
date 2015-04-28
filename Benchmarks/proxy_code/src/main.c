#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "benchmark.h"

// Command line options
#define OPTSTRING "vt:u:n:m:"
static struct option long_opts[] = {
  {"threads", optional_argument, NULL, 'i'},
  {"sleep_us", optional_argument, NULL, 'u'},
  {"loop_kcycles", optional_argument, NULL, 'n'},
  {"macro_cycles", optional_argument, NULL, 'm'},
  {"verbose",   optional_argument, NULL, 'v'}
};

struct arguments {
  int threads;
  int sleep_us;
  int loop_kcycles;
  int macro_cycles;
  int verbose;
};

void print_usage(char *progname) {
  fprintf(stderr, "Simple multi-threaded assembly benchmark\n");
  fprintf(stderr, "Usage: %s [OPTIONS]...\n\n", progname);
  fprintf(stderr, "\t-t [threads]\tSpecify thread count\n");
  fprintf(stderr, "\t-u [sleep useconds]\tSpecify cycle sleep duration\n");
  fprintf(stderr, "\t-n [loop kcycles] Specify iterations of empty loop\n");
  fprintf(stderr, "\t-m [macro cycles] Specify iterations of outer loop\n");
  fprintf(stderr, "\t-v\t\tEnable verbose mode. Useful for Debugging\n");
}


int main(int argc, char *argv[]) {
  // Parse command line arguments
  struct arguments args = {4, 500, 100, 10000, 0}; // Default values
  int optc;
  while ((optc = getopt_long(argc, argv, OPTSTRING, long_opts, NULL)) != EOF) {
    switch (optc) {
      case 't': {
        args.threads = atoi(optarg);
        if (args.threads < 1) {
          print_usage(argv[0]);
          exit(EXIT_FAILURE);
        }
        break;
      }
      case 'u': {
        args.sleep_us = atoi(optarg);
        if (args.sleep_us < 1) {
          print_usage(argv[0]);
          exit(EXIT_FAILURE);
        }
        break;
      }
      case 'n': {
        args.loop_kcycles = atoi(optarg);
        if (args.loop_kcycles < 1) {
          print_usage(argv[0]);
          exit(EXIT_FAILURE);
        }
        break;
      }
      case 'm': {
        args.macro_cycles = atoi(optarg);
        if (args.macro_cycles < 1) {
          print_usage(argv[0]);
          exit(EXIT_FAILURE);
        }
        break;
      }
      case 'v': {
        args.verbose = 1;
        break;
      }
      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  // Print out settings
  printf("%s invoked. Settings:\n", argv[0]);
  printf("\tThreads %d\n\tVerbose: %d\n", args.threads, args.verbose);
  run_benchmark(args.threads, args.sleep_us, args.loop_kcycles, args.macro_cycles, args.verbose);
  return 0;
}
