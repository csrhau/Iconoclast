#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "benchmark.h"

// Command line options
#define OPTSTRING "vj:p:"
static struct option long_opts[] = {
  {"jobs", optional_argument, NULL, 'j'},
  {"policy", optional_argument, NULL, 'p'},
  {"verbose",   optional_argument, NULL, 'v'}
};

struct arguments {
  int jobs;
  int verbose;
  int policy;
};

void print_usage(char *progname) {
  fprintf(stderr, "Simple multi-threaded assembly benchmark\n");
  fprintf(stderr, "Usage: %s [OPTIONS]...\n\n", progname);
  fprintf(stderr, "\t-j [jobs]\tSpecify instruction count\n");
  fprintf(stderr, "\t-p [policy] (1 - Round Robin, 2 - Packed \n");
  fprintf(stderr, "\t-v\t\tEnable verbose mode. Useful for Debugging\n");
}

int main(int argc, char *argv[]) {
  // Parse command line arguments
  struct arguments args = {1, 0}; // Default values
  int optc;
  while ((optc = getopt_long(argc, argv, OPTSTRING, long_opts, NULL)) != EOF) {
    switch (optc) {
      case 'j': {
        args.jobs = atoi(optarg);
        break;
      }
      case 'p': {
        args.policy = atoi(optarg);
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
  if (args.jobs < 1) {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
  }
  if (args.policy < 1 || args.policy > 2) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // Print out settings
  printf("%s invoked. Settings:\n", argv[0]);
  printf("\t Jobs %d\n\tPolicy %d\n\tVerbose: %d\n", args.jobs, args.policy, args.verbose);
  run_benchmark(args.jobs, args.policy, NOP);
  return 0;
}
