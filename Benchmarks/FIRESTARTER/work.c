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


#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

/*
 * Header for local functions
 */
#include "work.h"
#include "cpu.h"

/*
 * low load function
 */
int low_load_function(unsigned long long addrHigh, unsigned int period) __attribute__((noinline));
int low_load_function(unsigned long long addrHigh, unsigned int period)
{
   int nap;

   nap = period/100;
   while (*((unsigned long long*)addrHigh)==0)usleep(nap);

   return 0;
}

/*
 * function that performs the stress test
 */
inline void _work(volatile mydata_t* data, unsigned long long *high)
{
   unsigned int i;
 
   //start worker threads
   for (i=0;i<data->num_threads;i++)
   {
      data->ack=0;
      data->threaddata[i].addrHigh=(unsigned long long) high;
      data->thread_comm[i]=THREAD_WORK;
      while (!data->ack);
   }
   data->ack=0;
}

/*
 * loop for additional worker threads
 * communicating with master thread using shared variables
 */
void *thread(void *threaddata)
{
  int id= ((threaddata_t *) threaddata)->thread_id;
  volatile mydata_t* global_data = ((threaddata_t *) threaddata)->data; //communication with master thread
  threaddata_t* mydata = (threaddata_t*)threaddata;
  unsigned int tmp = 0;
  unsigned long long old=THREAD_STOP;  
 
  /* wait untill master thread starts initialization */
  while (global_data->thread_comm[id]!=THREAD_INIT);

  while(1)
  {
     switch (global_data->thread_comm[id]){
       case THREAD_INIT: // allocate and initialize memory
          if (old!=THREAD_INIT)  {
	    old=THREAD_INIT;

  	    /* set affinity  */
  	    #if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
  	    cpu_set(((threaddata_t *) threaddata)->cpu_id); 
  	    #endif

	    /* allocate memory */
  	    if(mydata->buffersizeMem){
		mydata->bufferMem = _mm_malloc(mydata->buffersizeMem,mydata->alignment);
		mydata->addrMem=(unsigned long long)(mydata->bufferMem);
  	    }
  	    if (mydata->bufferMem == NULL) global_data->ack=THREAD_INIT_FAILURE;
  	    else global_data->ack=id+1;

	    /* call init function */
            switch (mydata->FUNCTION)
            {
	    case FUNC_HSW_COREI_FMA_1T:
		tmp=init_hsw_corei_fma_1t(mydata->addrMem);
		break;
	    case FUNC_HSW_COREI_FMA_2T:
		tmp=init_hsw_corei_fma_2t(mydata->addrMem);
		break;
	    case FUNC_HSW_XEONEP_FMA_1T:
		tmp=init_hsw_xeonep_fma_1t(mydata->addrMem);
		break;
	    case FUNC_HSW_XEONEP_FMA_2T:
		tmp=init_hsw_xeonep_fma_2t(mydata->addrMem);
		break;
	    case FUNC_SNB_COREI_AVX_1T:
		tmp=init_snb_corei_avx_1t(mydata->addrMem);
		break;
	    case FUNC_SNB_COREI_AVX_2T:
		tmp=init_snb_corei_avx_2t(mydata->addrMem);
		break;
	    case FUNC_SNB_XEONEP_AVX_1T:
		tmp=init_snb_xeonep_avx_1t(mydata->addrMem);
		break;
	    case FUNC_SNB_XEONEP_AVX_2T:
		tmp=init_snb_xeonep_avx_2t(mydata->addrMem);
		break;
	    case FUNC_NHM_COREI_SSE2_1T:
		tmp=init_nhm_corei_sse2_1t(mydata->addrMem);
		break;
	    case FUNC_NHM_COREI_SSE2_2T:
		tmp=init_nhm_corei_sse2_2t(mydata->addrMem);
		break;
	    case FUNC_NHM_XEONEP_SSE2_1T:
		tmp=init_nhm_xeonep_sse2_1t(mydata->addrMem);
		break;
	    case FUNC_NHM_XEONEP_SSE2_2T:
		tmp=init_nhm_xeonep_sse2_2t(mydata->addrMem);
		break;
	    case FUNC_BLD_OPTERON_FMA4_1T:
		tmp=init_bld_opteron_fma4_1t(mydata->addrMem);
		break;
            default:
                fprintf(stderr,"Error: unknown function %i\n",mydata->FUNCTION);
                pthread_exit(NULL);
	    }
            if (tmp != EXIT_SUCCESS){
		fprintf(stderr,"Error in function %i\n",mydata->FUNCTION);
         	pthread_exit(NULL);
            } 

	  }
	  else tmp=100;while(tmp>0) tmp--;
	  break; // end case THREAD_INIT
       case THREAD_WORK: // perform stress test
          if (old!=THREAD_WORK) 
          {
            old=THREAD_WORK;
            global_data->ack=id+1;

	    /* will be terminated by watchdog 
             * watchdog also alters mydata->addrHigh to switch between high and low load function
             */
	    while(1){
		/* call high load function */
		switch (mydata->FUNCTION)
		{
		    case FUNC_HSW_COREI_FMA_1T:
			tmp=asm_work_hsw_corei_fma_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_HSW_COREI_FMA_2T:
			tmp=asm_work_hsw_corei_fma_2t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_HSW_XEONEP_FMA_1T:
			tmp=asm_work_hsw_xeonep_fma_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_HSW_XEONEP_FMA_2T:
			tmp=asm_work_hsw_xeonep_fma_2t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_SNB_COREI_AVX_1T:
			tmp=asm_work_snb_corei_avx_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_SNB_COREI_AVX_2T:
			tmp=asm_work_snb_corei_avx_2t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_SNB_XEONEP_AVX_1T:
			tmp=asm_work_snb_xeonep_avx_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_SNB_XEONEP_AVX_2T:
			tmp=asm_work_snb_xeonep_avx_2t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_NHM_COREI_SSE2_1T:
			tmp=asm_work_nhm_corei_sse2_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_NHM_COREI_SSE2_2T:
			tmp=asm_work_nhm_corei_sse2_2t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_NHM_XEONEP_SSE2_1T:
			tmp=asm_work_nhm_xeonep_sse2_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_NHM_XEONEP_SSE2_2T:
			tmp=asm_work_nhm_xeonep_sse2_2t(mydata->addrMem,mydata->addrHigh);
			break;
		    case FUNC_BLD_OPTERON_FMA4_1T:
			tmp=asm_work_bld_opteron_fma4_1t(mydata->addrMem,mydata->addrHigh);
			break;
		    default:
			fprintf(stderr,"Error: unknown function %i\n",mydata->FUNCTION);
			pthread_exit(NULL);
			break;
		}
		if (tmp != EXIT_SUCCESS){
		    fprintf(stderr,"Error in function %i\n",mydata->FUNCTION);
         	    pthread_exit(NULL);
		}

		/* call low load function */
		low_load_function(mydata->addrHigh,mydata->period);
             }

          }
          else tmp=100;while(tmp>0) tmp--;
          break; //end case THREAD_WORK
       case THREAD_WAIT:
          if (old!=THREAD_WAIT)
          {
            old=THREAD_WAIT;
            global_data->ack=id+1;
          }
          else tmp=100;while(tmp>0) tmp--;
          break;
       case THREAD_STOP: // exit
       default:
         pthread_exit(NULL);
    }
  }
}

