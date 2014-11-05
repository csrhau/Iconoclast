#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

#include "rapl_monitor.h"


struct arguments {
  bool verbose;
  unsigned interval;
};

static struct arguments * sh_args;

struct option options[] = {
  {"verbose", no_argument, NULL, 'v'},
  {"interval", required_argument, NULL, 'i'}
};

struct resources {
  struct PowerState pstate;
  struct timeval start;
  struct timeval end;
  struct timeval elapsed;
  struct rusage usage;
  int status;
};


typedef void (*sighandler)(int);

// globals
struct resources res;
timer_t timerid;

char * const* get_args(int argc, char **argv, struct arguments* args) {
  int opt;
  while ((opt = getopt_long(argc, argv, "+vi:", options, (int *) 0)) != EOF) {
    switch(opt) {
      case 'v':
        args->verbose = true;
        break;
      case 'i':
        args->interval = atoi(optarg);
        break;
      default: 
        fprintf(stderr, "Unable to parse arguments!\n");
        exit(EXIT_FAILURE);
        break;
    }
  }
  return (char * const*) &argv[optind];
}

void summarize(struct resources * const res, bool verbose) {
    printf("Real Time:\t%ldm  %02lds %ldus\n",  // m:s
           res->elapsed.tv_sec / 60,
           res->elapsed.tv_sec % 60,
           res->elapsed.tv_usec);
    printf("Sys Time:\t%lds %ldus\n",
		       res->usage.ru_stime.tv_sec,
		       res->usage.ru_stime.tv_usec);
	  printf ("User Time:\t%lds %ldus\n",
		       res->usage.ru_utime.tv_sec,
		       res->usage.ru_utime.tv_usec);
    printf("PKG Energy:\t%fJ\n", res->pstate.pkg_pwr);
    printf("PP0 Energy:\t%fJ\n", res->pstate.pp0_pwr);

    double secs = res->elapsed.tv_sec + res->elapsed.tv_usec * 1.0e-6;
    if (secs > 0) {
      printf("PKG Power:\t%fW\n", res->pstate.pkg_pwr / secs);
    }
    if (verbose) {
      printf("PKG Register:\t%ld\n", res->pstate.pkg_reg_value);
      printf("PP0 Register:\t%ld\n", res->pstate.pp0_reg_value);
    }
}


void refresh(int sig) {
  (void) (sig) ; // Unused parameter
  rapl_refresh(&res.pstate);
  if (sh_args->verbose) {
    summarize(&res, true);
  }
}

void resuse_start(struct resources * const res, int refresh_interval) {

  // Try to open the msr file
  res->pstate.msr_fh = open("/dev/cpu/0/msr", O_RDONLY);
  if (res->pstate.msr_fh < 0) {
    fprintf(stderr, "Unable to open MSR file for reading!\n");
    exit(EXIT_FAILURE); 
  }

  // Schedule power refresh interrupts
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sa.sa_sigaction = refresh;

  if (sigaction(SIGUSR1, &sa, NULL) == -1) {
    printf("SIGACTION FAILED!\n");
    exit(1);
  }

  struct sigevent sev;
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIGUSR1;
  sev.sigev_value.sival_ptr = &timerid; 
  struct itimerspec its;
  if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) != 0) {
    printf("TIMER CREATE ERROR!\n");
    exit(1);
  }

  its.it_value.tv_sec = refresh_interval;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = refresh_interval;
  its.it_interval.tv_nsec = 0;
  timer_settime(timerid, 0, &its, NULL);
  // Take initial readings
  rapl_init(&res->pstate);
  // Record start time
  gettimeofday(&res->start, (struct timezone *) NULL);
}


int resuse_end(pid_t pid, struct resources * const res) {
  pid_t caught;
  // Ignore signals, but don't ignore the children. When wait3
  // returns, set the time the command finnished
  while ((caught = wait3(&res->status, 0, &res->usage)) != pid) {}

  // Do time-sensitive things first
  gettimeofday(&res->end, NULL);
  rapl_finalize(&res->pstate);
  // Disable power signal?
  // Get current power
  // Everything after this line depends on captured metrics
  res->elapsed.tv_sec = res->end.tv_sec - res->start.tv_sec;
  res->elapsed.tv_usec = res->end.tv_usec - res->start.tv_usec;
  // Manually carry a one from the seconds field
  if (res->end.tv_usec < res->start.tv_usec) {
    res->elapsed.tv_usec += 1000000;
    res->elapsed.tv_sec -= 1;
  }
  return 1;
}


void run_command(char * const* cmd, 
                 struct resources * const res,
                 struct arguments * const args)
{
  pid_t pid;
  // Record start of run
  gettimeofday(&(res->start), NULL);
  resuse_start(res, args->interval);
  pid = fork();
  if (pid < 0) { // failure
    fprintf(stderr, "Unable to fork process\n");
    exit(EXIT_FAILURE); 
  } else if (pid == 0) { // child
    execvp(cmd[0], cmd);
    // Should never return
    fprintf(stderr, "Unable to run child process. Check your command\n");
    exit(EXIT_FAILURE); 
  } else { //parent
    // Have signals kill the child but not the self if possible
    sighandler interrupt_signal, quit_signal;
    interrupt_signal = signal(SIGINT, SIG_IGN);
    quit_signal = signal(SIGQUIT, SIG_IGN);
    resuse_end(pid, res); 
    // Re-enable signals
    signal(SIGINT, interrupt_signal);
    signal(SIGQUIT, quit_signal);
  }
}


int main(int argc, char **argv) {
  struct arguments args = {false, 15};
  sh_args = &args;
  char * const* command_line = get_args(argc, argv, &args);
  if (args.verbose) {
    // Print sub-command to be run
    char * const* arg = command_line;
    printf("Command line:\t$");
    while (*arg) {
      printf("%s ", *arg);
      ++arg;
    }
    printf("\n");
  }

  run_command(command_line, &res, &args);
  summarize(&res, false);
}
