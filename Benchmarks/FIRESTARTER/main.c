/******************************************************************************
 * FIRESTARTER - A Processor Stress Test Utility 
 * Copyright (C) 2014 TU Dresden, Center for Information Services and High
 * Performance Computing
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * Contact: daniel.hackenberg@tu-dresden.de
 *****************************************************************************/


/* current version */
#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define COPYRIGHT_YEAR 2014

#define _GNU_SOURCE
#if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
#include <sched.h>
#endif

#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

#include <sys/time.h>

/* Mac OS compatibility */
#ifdef __MACH__
#include <mach/mach_time.h>
#define CLOCK_REALTIME 0
#endif

/*
 * Header for local functions
 */
#include "work.h"
#include "cpu.h"

mydata_t *mdp;			/* global data structure */
cpu_info_t *cpuinfo = NULL; 	/* data structure for hardware detection */
unsigned long long HIGH=1;  	/* shared variable that specifies load level */
int ALIGNMENT = 64; 		/* alignment of buffers and data structures */
unsigned int verbose = 1;	/* enable/disable output to stdout */

/*
 * FIRESTARTER configuration, determined by evaluate_environment function 
 */
unsigned long long BUFFERSIZEMEM,RAMBUFFERSIZE;
unsigned int BUFFERSIZE[3]; 
unsigned int NUM_THREADS = 0;
int FUNCTION = FUNC_NOT_DEFINED;

/*
 * timeout and load characteristics as defind by -t, -p, and -l 
 */
long TIMEOUT = 0, PERIOD = 100, LOAD = 100;

/*
 * pointer for CPU bind argument (-b | --bind)
 */
char* bind=NULL;

/* Mac OS compatibility */
#ifdef __MACH__
mach_timebase_info_data_t info;
double ns_per_tick;
#endif

/*
 * worker threads
 */
pthread_t *threads;	

/*
 * CPU bindings of threads
 */
#if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
cpu_set_t cpuset;
#endif
unsigned long long *cpu_bind;

/*
 * watchdog timer
 */
typedef struct watchdog_args {
    unsigned long long *high;
    pid_t pid;
    useconds_t period;
    useconds_t load;
    unsigned int timeout;
} watchdog_arg_t;
watchdog_arg_t watchdog_arg;

/* exit with zero returncode on sigterm */
static void sigterm_handler()
{
    exit(EXIT_SUCCESS);
}

/* Mac OS compatibility - clock_gettime is not implemented on OSX */
#ifdef __MACH__
static int clock_gettime(int id, struct timespec* t) {
    uint64_t ts,sec,nsec;

    ts = (uint64_t) ((double)mach_absolute_time() * ns_per_tick);
    sec = ts/1000000000;
    nsec = ts - sec*1000000000;
    t->tv_sec = (time_t) sec;
    t->tv_nsec = (long) nsec;
    return id;
}
#endif

/* 
 * coordinates high load and low load phases
 * stops FIRESTARTER when timeout is reached 
 */
static void *watchdog_timer(void *arg)
{
    sigset_t signal_mask;
    register long long timeout, time, period, load, idle, advance,load_reduction,idle_reduction;
    register unsigned long long *high;
    struct timespec start_ts,current;

    /* Mac OS compatibility */
    #ifdef __MACH__
    mach_timebase_info(&info);
    ns_per_tick = (double)info.numer / (double)info.denom;
    #endif

    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    period=((watchdog_arg_t *) arg)->period;
    load=((watchdog_arg_t *) arg)->load;
    idle=period-load;
    timeout=((watchdog_arg_t *) arg)->timeout;
    high=((watchdog_arg_t *) arg)->high;

    clock_gettime(CLOCK_REALTIME,&start_ts);

    time=0;
    while (period > 0) {
      clock_gettime(CLOCK_REALTIME,&current);
      advance=((current.tv_sec-start_ts.tv_sec)*1000000 + (current.tv_nsec-start_ts.tv_nsec)/1000) % period;
      load_reduction=(load * advance) / period;
      idle_reduction=advance-load_reduction;

      usleep(load - load_reduction);
      *high = 0; // interupt high load routine
      usleep(idle - idle_reduction);
      *high = 1; // interupt low load routine
      time+=period;
      if ((timeout > 0) && (time/1000000 >= timeout)){
        kill(((watchdog_arg_t *) arg)->pid, SIGTERM);
        pthread_exit(0);
      }
    }

    if (timeout > 0) {
        sleep(timeout);
        kill(((watchdog_arg_t *) arg)->pid, SIGTERM);
    }
    // do not terminate the main thread                                         
    // only in case the watchdog is not running                                 
    sleep(UINT_MAX);
    return(0);
}

/*
 * initialize data structures
 */
static void *init()
{
    unsigned int i,t;

    #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
    cpu_set(cpu_bind[0]);
    #endif
    mdp->cpuinfo = cpuinfo;

    BUFFERSIZEMEM = sizeof(char) * (2 * BUFFERSIZE[0] + BUFFERSIZE[1] + BUFFERSIZE[2] + RAMBUFFERSIZE +
                    ALIGNMENT + 2 * sizeof(unsigned long long));

    if (BUFFERSIZEMEM <= 0) {
        fprintf(stderr, "Error: Determine BUFFERSIZEMEM failed\n");
        fflush(stderr);
        exit(127);
    }

    if ((NUM_THREADS > mdp->cpuinfo->num_cpus) || (NUM_THREADS == 0))
        NUM_THREADS = mdp->cpuinfo->num_cpus;

    threads = _mm_malloc(NUM_THREADS * sizeof(pthread_t), ALIGNMENT);
    mdp->thread_comm = _mm_malloc(NUM_THREADS * sizeof(int), ALIGNMENT);
    mdp->threaddata = _mm_malloc(NUM_THREADS * sizeof(threaddata_t), ALIGNMENT);
    mdp->num_threads = NUM_THREADS;
    if ((threads == NULL) || (mdp->thread_comm == NULL) || (mdp->threaddata == NULL)) {
        fprintf(stderr,"Error: Allocation of structure mydata_t failed\n");
        fflush(stderr);
        exit(127);
    }

    // create worker threads
    for (t = 0; t < NUM_THREADS; t++) {
        mdp->ack = 0;
        mdp->threaddata[t].thread_id = t;
        mdp->threaddata[t].cpu_id = cpu_bind[t];
        mdp->threaddata[t].data = mdp;
        mdp->threaddata[t].buffersizeMem = BUFFERSIZEMEM;
        mdp->threaddata[t].alignment = ALIGNMENT;
        mdp->threaddata[t].FUNCTION = FUNCTION;
	mdp->threaddata[t].period = PERIOD;
	mdp->thread_comm[t] = THREAD_INIT;
        i=pthread_create(&(threads[t]), NULL, thread,(void *) (&(mdp->threaddata[t])));
        while (!mdp->ack);
	if (mdp->ack == THREAD_INIT_FAILURE) {
            fprintf(stderr,"Error: Initialization of threads failed\n");
            fflush(stderr);
            exit(127);
        }
    }
    mdp->ack = 0;

    #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
    cpu_set(cpu_bind[0]);
    #endif

    /* wait for threads to complete their initialization */
    for (t = 0; t < NUM_THREADS; t++) {
        mdp->ack = 0;
        mdp->thread_comm[t] = THREAD_WAIT;
        while (!mdp->ack);
    }
    mdp->ack = 0;

    if (verbose){
	if ((num_packages() != -1) && (num_cores_per_package() != -1) && (num_threads_per_core() != -1))
            printf ("  num_packages: %i, %i cores per package, %i threads per core\n",
                    num_packages(), num_cores_per_package(),num_threads_per_core());
        printf("  using %i threads\n", NUM_THREADS);
	#if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
	for (i = 0; i < NUM_THREADS; i++)
	    if ((get_pkg(cpu_bind[i]) != -1) && (get_core_id(cpu_bind[i]) != -1))
                printf("    - Thread %i runs on CPU %llu, core %i in package: %i\n",
                 i, cpu_bind[i], get_core_id(cpu_bind[i]),get_pkg(cpu_bind[i]));
	#endif
	printf("\n");fflush(stdout);
    }

    return (void *) mdp;
}

/*
 * detect hardware configuration and setup FIRESTARTER accordingly
 */
static void evaluate_environment()
{
    unsigned int i;
    #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
    unsigned int j = 0;
    #endif
    char arch[16];

    cpuinfo = (cpu_info_t *) _mm_malloc(sizeof(cpu_info_t), 64);
    init_cpuinfo(cpuinfo, verbose);

    if (bind==NULL){ //use all CPUs if not defined otherwise
      #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
      CPU_ZERO(&cpuset);
      for (i = 0; i < cpuinfo->num_cpus; i++) {
        if (cpu_allowed(i)) {
            CPU_SET(i, &cpuset);
            NUM_THREADS++;
        }
      }
      #else
      NUM_THREADS = cpuinfo->num_cpus;
      #endif
    }
    #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
    else { // parse CPULIST for binding
      char *p,*q,*r,*s,*t;
      int p_val,r_val,s_val,error=0;

      errno=0; 
      p=strdup(bind);
      while(p!=NULL){
        q=strstr(p,",");if (q) {*q='\0';q++;}
        s=strstr(p,"/");if (s) {*s='\0';s++;s_val=(int)strtol(s,&t,10);if ((errno) || ((strcmp(t,"\0") && (t[0] !=','))) ) error++;}
        r=strstr(p,"-");if (r) {*r='\0';r++;r_val=(int)strtol(r,&t,10);if ((errno) || ((strcmp(t,"\0") && (t[0] !=',') && (t[0] !='/'))) ) error++;}
        p_val=(int)strtol(p,&t,10); if ((errno) || (p_val < 0) || (strcmp(t,"\0"))) error++;
        if(error){
          fprintf(stderr, "Error: invalid symbols in CPU list: %s\n",bind);
          fflush(stderr);
          exit(127);
        }
        if ((s) && (s_val<=0)) {
          fprintf(stderr, "Error: s has to be >= 0 in x-y/s expressions of CPU list: %s\n",bind);
          fflush(stderr);
          exit(127);
        }
        if ((r) && (r_val < p_val)) {
          fprintf(stderr, "Error: y has to be >= x in x-y expressions of CPU list: %s\n",bind);
          fflush(stderr);
          exit(127);
        }
        if ((s)&&(r)) for (i=p_val;(int)i<=r_val;i+=s_val) {if (cpu_allowed(i)) {CPU_SET(i,&cpuset);NUM_THREADS++;}}
        else if (r) for (i=p_val;(int)i<=r_val;i++) {if (cpu_allowed(i)) {CPU_SET(i,&cpuset);NUM_THREADS++;}}
        else if (cpu_allowed(p_val)) {CPU_SET(p_val,&cpuset);NUM_THREADS++;}
        p=q;
      }
      free(p);
    }
    #endif

    cpu_bind = (unsigned long long *) calloc((NUM_THREADS + 1), sizeof(unsigned long long));
    if (NUM_THREADS == 0) {
      fprintf(stderr, "Error: found no useable CPUs!\n");
      fflush(stderr);
      exit(127);
    } 
    #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
    else {
      for (i = 0; i < cpuinfo->num_cpus; i++) {
        if (CPU_ISSET(i, &cpuset)) {
           cpu_bind[j] = i;
           j++;
        }
      }
    }
    #endif

    mdp = (mydata_t *) _mm_malloc(sizeof(mydata_t), ALIGNMENT);
    if (mdp == 0) {
        fprintf(stderr,"Error: Allocation of structure mydata_t failed (1)\n");
        fflush(stderr);
        exit(127);
    }
    memset(mdp, 0, sizeof(mydata_t));

    get_architecture(arch, sizeof(arch));
    if (strcmp(arch, "x86_64")) {
        fprintf(stderr,"Error: wrong architecture: %s, x86_64 required \n", arch);
        exit(1);
    }

    if (!feature_available("SSE2")) {
        fprintf(stderr, "Error: SSE2 not supported!\n");
        exit(1);
    }

    if ((strcmp("GenuineIntel", cpuinfo->vendor) == 0) || 
        (strcmp("AuthenticAMD", cpuinfo->vendor) == 0)) 
    {
      switch (cpuinfo->family) {
	case 6:
          switch (cpuinfo->model) {
	    case 60:
	       if (feature_available("FMA")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_HSW_COREI_FMA_1T;
		  if (num_threads_per_core() == 2) FUNCTION = FUNC_HSW_COREI_FMA_2T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: FMA \n");
           	  exit(1);
               }
	       break;
	    case 63:
	       if (feature_available("FMA")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_HSW_XEONEP_FMA_1T;
		  if (num_threads_per_core() == 2) FUNCTION = FUNC_HSW_XEONEP_FMA_2T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: FMA \n");
           	  exit(1);
               }
	       break;
	    case 42:
	    case 58:
	       if (feature_available("AVX")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_SNB_COREI_AVX_1T;
		  if (num_threads_per_core() == 2) FUNCTION = FUNC_SNB_COREI_AVX_2T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: AVX \n");
           	  exit(1);
               }
	       break;
	    case 45:
	    case 62:
	       if (feature_available("AVX")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_SNB_XEONEP_AVX_1T;
		  if (num_threads_per_core() == 2) FUNCTION = FUNC_SNB_XEONEP_AVX_2T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: AVX \n");
           	  exit(1);
               }
	       break;
	    case 30:
	    case 37:
	       if (feature_available("SSE2")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_NHM_COREI_SSE2_1T;
		  if (num_threads_per_core() == 2) FUNCTION = FUNC_NHM_COREI_SSE2_2T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: SSE2 \n");
           	  exit(1);
               }
	       break;
	    case 26:
	    case 44:
	       if (feature_available("SSE2")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_NHM_XEONEP_SSE2_1T;
		  if (num_threads_per_core() == 2) FUNCTION = FUNC_NHM_XEONEP_SSE2_2T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: SSE2 \n");
           	  exit(1);
               }
	       break;
	    default:
	       fprintf(stderr, "Error: processor model (family %i, model %i) is not supported by this version of FIRESTARTER! Check project website for updates.\n",cpuinfo->family,cpuinfo->model);
               exit(1);
	       break;
          }
	  break;
	case 21:
          switch (cpuinfo->model) {
	    case 1:
	    case 2:
	    case 3:
	       if (feature_available("FMA4")) {
		  if (num_threads_per_core() == 1) FUNCTION = FUNC_BLD_OPTERON_FMA4_1T;
		  if (FUNCTION == FUNC_NOT_DEFINED) {
           		fprintf(stderr, "Error: no code path for %i threads per core!\n",num_threads_per_core());
           		exit(1);
        	  }
               }
	       if (FUNCTION == FUNC_NOT_DEFINED) {
           	  fprintf(stderr, "Error: required instruction set not supported! Supported instruction set(s) for this processor: FMA4 \n");
           	  exit(1);
               }
	       break;
	    default:
	       fprintf(stderr, "Error: processor model (family %i, model %i) is not supported by this version of FIRESTARTER! Check project website for updates.\n",cpuinfo->family,cpuinfo->model);
               exit(1);
	       break;
          }
	  break;
	default:
           fprintf(stderr, "Error: family %i processors are not supported by this version of FIRESTARTER! Check project website for updates.\n",cpuinfo->family);
           exit(1);
	  break;
      }
    }
    else {
        fprintf(stderr, "Error: %s processors not supported by this version of FIRESTARTER!\n",cpuinfo->vendor);
        exit(1);
    }


    switch (FUNCTION) {
    case FUNC_HSW_COREI_FMA_1T:
	if (verbose) printf("\n  Taking FMA Path optimized for Haswell - 1 thread(s) per core");


	BUFFERSIZE[0] = 32768;
	BUFFERSIZE[1] = 262144;
	BUFFERSIZE[2] = 1572864;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_HSW_COREI_FMA_2T:
	if (verbose) printf("\n  Taking FMA Path optimized for Haswell - 2 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 131072;
	BUFFERSIZE[2] = 786432;
	RAMBUFFERSIZE = 52428800;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_HSW_XEONEP_FMA_1T:
	if (verbose) printf("\n  Taking FMA Path optimized for Haswell-EP - 1 thread(s) per core");


	BUFFERSIZE[0] = 32768;
	BUFFERSIZE[1] = 262144;
	BUFFERSIZE[2] = 2621440;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_HSW_XEONEP_FMA_2T:
	if (verbose) printf("\n  Taking FMA Path optimized for Haswell-EP - 2 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 131072;
	BUFFERSIZE[2] = 1310720;
	RAMBUFFERSIZE = 52428800;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_SNB_COREI_AVX_1T:
	if (verbose) printf("\n  Taking AVX Path optimized for Sandy Bridge - 1 thread(s) per core");


	BUFFERSIZE[0] = 32768;
	BUFFERSIZE[1] = 262144;
	BUFFERSIZE[2] = 1572864;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_SNB_COREI_AVX_2T:
	if (verbose) printf("\n  Taking AVX Path optimized for Sandy Bridge - 2 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 131072;
	BUFFERSIZE[2] = 786432;
	RAMBUFFERSIZE = 52428800;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_SNB_XEONEP_AVX_1T:
	if (verbose) printf("\n  Taking AVX Path optimized for Sandy Bridge-EP - 1 thread(s) per core");


	BUFFERSIZE[0] = 32768;
	BUFFERSIZE[1] = 262144;
	BUFFERSIZE[2] = 2621440;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_SNB_XEONEP_AVX_2T:
	if (verbose) printf("\n  Taking AVX Path optimized for Sandy Bridge-EP - 2 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 131072;
	BUFFERSIZE[2] = 1310720;
	RAMBUFFERSIZE = 52428800;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_NHM_COREI_SSE2_1T:
	if (verbose) printf("\n  Taking SSE2 Path optimized for Nehalem - 1 thread(s) per core");


	BUFFERSIZE[0] = 32768;
	BUFFERSIZE[1] = 262144;
	BUFFERSIZE[2] = 1572864;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_NHM_COREI_SSE2_2T:
	if (verbose) printf("\n  Taking SSE2 Path optimized for Nehalem - 2 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 131072;
	BUFFERSIZE[2] = 786432;
	RAMBUFFERSIZE = 52428800;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_NHM_XEONEP_SSE2_1T:
	if (verbose) printf("\n  Taking SSE2 Path optimized for Nehalem-EP - 1 thread(s) per core");


	BUFFERSIZE[0] = 32768;
	BUFFERSIZE[1] = 262144;
	BUFFERSIZE[2] = 2097152;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_NHM_XEONEP_SSE2_2T:
	if (verbose) printf("\n  Taking SSE2 Path optimized for Nehalem-EP - 2 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 131072;
	BUFFERSIZE[2] = 1048576;
	RAMBUFFERSIZE = 52428800;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_BLD_OPTERON_FMA4_1T:
	if (verbose) printf("\n  Taking FMA4 Path optimized for Bulldozer - 1 thread(s) per core");


	BUFFERSIZE[0] = 16384;
	BUFFERSIZE[1] = 1048576;
	BUFFERSIZE[2] = 786432;
	RAMBUFFERSIZE = 104857600;
	if (verbose){
	   printf("\n  Used buffersizes per thread:\n");
           for (i = 0; i < cpuinfo->Cachelevels; i++) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
	   printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
	}
	break;
    case FUNC_NOT_DEFINED:
	fprintf(stderr, "Error: required instruction set not supported!\n");
	exit(1);
    default:
        fprintf(stderr, "Error: CPU not supported!\n");
        exit(1);
    }

}

static void show_help(void)
{
    printf("FIRESTARTER - A Processor Stress Test Utility, Version %i.%i\n",VERSION_MAJOR, VERSION_MINOR);
    printf("Copyright (C) %i TU Dresden, Center for Information Services and High Performance Computing\n",COPYRIGHT_YEAR);
    printf("This program comes with ABSOLUTELY NO WARRANTY; for details run `FIRESTARTER -w'.\n");
    printf("This is free software, and you are welcome to redistribute it\nunder certain conditions; run `FIRESTARTER -c' for details.\n");
    printf("\nUsage: FIRESTARTER [Options]\n"
           "\nOptions:\n\n"
           " -h         | --help             display usage information\n"
           " -v         | --version          display version information\n"
           " -c         | --copyright        display copyright information\n"
           " -w         | --warranty         display warranty information\n"
           " -q         | --quiet            disable output to stdout\n"
           " -t TIMEOUT | --timeout=TIMEOUT  set timeout (seconds) after which FIRESTARTER\n"
           "                                 terminates itself, default: no timeout\n"
           " -l LOAD    | --load=LOAD        set the percentage of high load to LOAD (%%),\n"
           "                                 default 100, valid values: 0 <= LOAD <= 100,\n"
           "                                 threads will be idle in the remaining time,\n"
           "                                 frequency of load changes is determined by -p\n"
           " -p PERIOD  | --period=PERIOD    set the interval length to PERIOD (ms),\n"
           "                                 default: 100, each interval contains a high\n"
           "                                 load and an idle phase, the percentage of \n"
           "                                 high load is defined by -l\n"
#if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
           " -b CPULIST | --bind=CPULIST     select certain CPUs\n"
           "                                 CPULIST format: \"x,y,z\", \"x-y\", \"x-y/step\",\n"
           "                                 and any combination of the above\n"
#endif
           "\nExamples:\n\n"
           "./FIRESTARTER                    - starts FIRESTARTER without timeout\n"
           "./FIRESTARTER -t 300             - starts a 5 minute run of FIRESTARTER\n"
           "./FIRESTARTER -l 50 -t 600       - starts a 10 minute run of FIRESTARTER with\n"
           "                                   50%% high load and 50%% idle time\n" 
           "./FIRESTARTER -l 75 -p 2000      - starts FIRESTARTER with an interval length\n"
           "                                   of 2 seconds, 1.5s high load and 0.5s idle\n" 
           "\n");
}

static void show_warranty(void)
{
    printf("FIRESTARTER - A Processor Stress Test Utility, Version %i.%i\n",VERSION_MAJOR, VERSION_MINOR);
    printf("Copyright (C) %i TU Dresden, Center for Information Services and High Performance Computing\n",COPYRIGHT_YEAR);
    printf("\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\n");
    printf("You should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n");
}

static void show_copyright(void)
{
    printf("FIRESTARTER - A Processor Stress Test Utility, Version %i.%i\n",VERSION_MAJOR, VERSION_MINOR);
    printf("Copyright (C) %i TU Dresden, Center for Information Services and High Performance Computing\n",COPYRIGHT_YEAR);
    printf("\nThis program is free software: you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of the License, or\n(at your option) any later version.\n\n");
    printf("You should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n");
}

static void show_version(void)
{
    printf("FIRESTARTER - A Processor Stress Test Utility, Version %i.%i\n",VERSION_MAJOR, VERSION_MINOR);
    printf("Copyright (C) %i TU Dresden, Center for Information Services and High Performance Computing\n",COPYRIGHT_YEAR);
}

int main(int argc, char *argv[])
{
    int c;
    static struct option long_options[] = {
		{"copyright",	no_argument, 		0, 'c'},
		{"help",	no_argument,		0, 'h'},
		{"version",	no_argument,		0, 'v'},
		{"warranty",	no_argument,		0, 'w'},
		{"quiet",	no_argument,		0, 'q'},
		{"bind",	required_argument,	0, 'b'},
		{"timeout",	required_argument,	0, 't'},
		{"load",	required_argument, 	0, 'l'},
		{"period",	required_argument,	0, 'p'},
		{0,         	0,                 	0,  0 }
	};
    while(1)
    {
	#if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
    	c = getopt_long(argc, argv, "chvwqb:t:l:p:", long_options, NULL);
	#else
	c = getopt_long(argc, argv, "chvwqt:l:p:", long_options, NULL);
	#endif
    	if(c == -1) break;

	errno = 0;
    	switch(c)
    	{
    	    case 'c':
    		show_copyright();
            	return EXIT_SUCCESS;
            case 'h':
            	show_help();
            	return EXIT_SUCCESS;
            case 'v':
            	show_version();
            	return EXIT_SUCCESS;
            case 'w':
            	show_warranty();
            	return EXIT_SUCCESS;
            case 'q':
            	verbose = 0;
            	break;
            case 'b':
            	bind=strdup(optarg);
            	break;
            case 't':
            	TIMEOUT = strtol(optarg,NULL,10);
		if ((errno != 0) || (TIMEOUT <= 0)){
			printf("Error: timeout out of range or not a number: %s\n",optarg);
			return EXIT_FAILURE;
		}
            	break;
            case 'l':
            	LOAD = strtol(optarg,NULL,10);
		if ((errno != 0) || (LOAD < 0) || (LOAD > 100)){
			printf("Error: load out of range or not a number: %s\n",optarg);
			return EXIT_FAILURE;
		}
            	break;
            case 'p':
            	PERIOD = strtol(optarg,NULL,10);
		if ((errno != 0) || (PERIOD <= 0)){
			printf("Error: period out of range or not a number: %s\n",optarg);
			return EXIT_FAILURE;
		}
            	break;
            case ':':	// Missing argument
            	return EXIT_FAILURE;
            case '?':	// Unknown option
            	return EXIT_FAILURE;            	
    	}
    }
    if(optind < argc)
    {
	printf("Error: too many parameters!\n");
	return EXIT_FAILURE;
    }

    PERIOD*=1000;
    LOAD = (PERIOD*LOAD)/100;
    if ((LOAD==PERIOD)||(LOAD==0)) PERIOD = 0; 	// disable interupts for 100% and 0% load case
    if (LOAD==0) HIGH=0;			// disable high load routine
	
    if (verbose){
	printf("FIRESTARTER - A Processor Stress Test Utility, Version %i.%i\n",VERSION_MAJOR, VERSION_MINOR);
	printf("Copyright (C) %i TU Dresden, Center for Information Services and High Performance Computing\n",COPYRIGHT_YEAR);
	printf("This program comes with ABSOLUTELY NO WARRANTY; for details run `FIRESTARTER -w'.\n");
	printf("This is free software, and you are welcome to redistribute it\nunder certain conditions; run `FIRESTARTER -c' for details.\n");
    }

    signal(SIGTERM, sigterm_handler);
    evaluate_environment();
    init();

    //start worker threads
    _work(mdp, &HIGH);

    //start watchdog
    watchdog_arg.pid = getpid();
    watchdog_arg.timeout = (unsigned int) TIMEOUT;
    watchdog_arg.period = (useconds_t) PERIOD;
    watchdog_arg.load = (useconds_t) LOAD;
    watchdog_arg.high = &HIGH;
    watchdog_timer(&watchdog_arg);

    return EXIT_SUCCESS;
}

