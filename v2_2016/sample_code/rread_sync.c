/*
 * Scale-Out NUMA Open Source License
 *
 * Copyright (c) 2017, Parallel Systems Architecture Lab, EPFL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the Parallel Systems Architecture Lab, EPFL,
 *   nor the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LAB,
 * EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Remote read test for libsonuma and SoftRMC
 *
 *  Authors: 
 *  	Stanko Novakovic <stanko.novakovic@epfl.ch>
 */

#include <vector>
#include <algorithm>

#include "sonuma.h"

#define ITERS           4096
#define BILLION 1000000000

#define SLOT_SIZE       64
#define OBJ_READ_SIZE   64

#define CTX_0           0

//#define USE_TIMER

using namespace std;

static __inline__ unsigned long long rdtsc(void) {
  unsigned long hi, lo;
  __asm__ __volatile__ ("xorl %%eax,%%eax\ncpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi)<<32) ;
}

int main(int argc, char **argv)
{
  rmc_wq_t *wq;
  rmc_cq_t *cq;

  int num_iter = (int)ITERS;

  if (argc != 4) {
    fprintf(stdout,"Usage: ./uBench <target_nid> <context_size> <buffer_size>\n");
    return 1;
  }
    
  int snid = atoi(argv[1]);
  uint64_t ctx_size = atoi(argv[2]);
  uint64_t buf_size = atoi(argv[3]);
  
  uint8_t *lbuff = NULL;
  uint8_t *ctx;
  uint64_t lbuff_slot;
  uint64_t ctx_offset;

  //local buffer
  kal_reg_lbuff(0, &lbuff, 0);
  fprintf(stdout, "Local buffer was mapped to address %p, number of pages is %d\n",
	  lbuff, buf_size/PAGE_SIZE);

  //context
  kal_reg_ctx(0, &ctx, 0);
  fprintf(stdout, "Ctx buffer was registered, ctx_size=%d, %d pages.\n",
	  ctx_size, ctx_size*sizeof(uint8_t) / PAGE_SIZE);

  kal_reg_wq(0, &wq);
  fprintf(stdout, "WQ was registered.\n");

  kal_reg_cq(0, &cq);
  fprintf(stdout, "CQ was registered.\n");
  
  fprintf(stdout,"Init done! Will execute %d WQ operations - SYNC! (snid = %d)\n",
	  num_iter, snid);

#ifdef USE_TIMER
  struct timespec start_time, end_time;
  uint64_t start_time_ns, end_time_ns;
  vector<uint64_t> stimes;
#else
  unsigned long long start, end;
#endif
  
  lbuff_slot = 0;
  
  for(size_t i = 0; i < num_iter; i++) {
    ctx_offset = (i * PAGE_SIZE) % ctx_size; // - PAGE_SIZE);

#ifdef USE_TIMER
    clock_gettime(CLOCK_MONOTONIC, &start_time);
#else
    start = rdtsc();
#endif

    rmc_rread_sync(wq, cq, lbuff_slot, snid, CTX_0, ctx_offset, OBJ_READ_SIZE);

#ifdef USE_TIMER
    clock_gettime(CLOCK_MONOTONIC, &end_time);
#else
    end = rdtsc();
#endif
    
#ifdef USE_TIMER
    start_time_ns = BILLION * start_time.tv_sec + start_time.tv_nsec;
    end_time_ns = BILLION * end_time.tv_sec + end_time.tv_nsec;
    
    stimes.insert(stimes.begin(), end_time_ns - start_time_ns);
    
    if(stimes.size() == 100) {
      long sum = 0;
      sort(stimes.begin(), stimes.end());
      for(int i=0; i<100; i++)
	sum += stimes[i];
      printf("[hophashtable_asy] min latency: %u ns; median latency: %u ns; 99th latency: %u ns; avg latency: %u ns\n", stimes[0], stimes[50], stimes[99], sum/100);
      
      while (!stimes.empty())
	stimes.pop_back();
    }
#else    
    printf("read this number: %u\n", ((uint32_t*)lbuff)[lbuff_slot]);
    printf("time to execute this op: %lf ns\n", ((double)end - start)/2.4);
#endif
  }
 
  return 0;
}
