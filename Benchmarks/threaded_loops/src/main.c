#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "benchmark.h"

// Command line options
#define OPTSTRING "vi:"
static struct option long_opts[] = {
  {"threads", optional_argument, NULL, 'i'},
  {"verbose",   optional_argument, NULL, 'v'}
};

struct arguments {
  int threads;
  int verbose;
};

void print_usage(char *progname) {
  fprintf(stderr, "Simple multi-threaded assembly benchmark\n");
  fprintf(stderr, "Usage: %s [OPTIONS]...\n\n", progname);
  fprintf(stderr, "\t-t [threads]\tSpecify thread count\n");
  fprintf(stderr, "\t-v\t\tEnable verbose mode. Useful for Debugging\n");
}

int main(int argc, char *argv[]) {
  // Parse command line arguments
  struct arguments args = {1, 0}; // Default values
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
  return 0;
}
