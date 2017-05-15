#define __STDC_FORMAT_MACROS
#include <vector>
#include <algorithm>

#include "sonuma.h"

#define ITERS           1000
#define BILLION 1000000000

#define SLOT_SIZE       64
#define OBJ_READ_SIZE   64

#define CTX_0           0

rmc_wq_t *wq;
rmc_cq_t *cq;

using namespace std;

static __inline__ unsigned long long rdtsc(void) {
  unsigned long hi, lo;
  __asm__ __volatile__ ("xorl %%eax,%%eax\ncpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi)<<32) ;
}

int main(int argc, char **argv) {
    int num_iter = (int)ITERS;

    if (argc != 4) {
        fprintf(stdout,"Usage: ./uBench <target_nid> <context_size> <buffer_size>\n");
        return 1;
    }
    
    int snid = atoi(argv[1]);
    uint64_t ctx_size = atoi(argv[2]);
    uint64_t buf_size = atoi(argv[3]);

    uint8_t *lbuff, *ctx;
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

    fprintf(stdout,"Init done! Will execute %d WQ operations - SYNC! (snid = %d)\n",
	    num_iter, snid);

    //uB kernel
    struct timespec start_time, end_time;
    uint64_t start_time_ns, end_time_ns;
    vector<uint64_t> stimes;
    unsigned long long start, end;
    
    for(size_t i = 0; i < 4096; i++) {
      ((uint32_t*)ctx)[(i*PAGE_SIZE)/4] = i;
	//clock_gettime(CLOCK_MONOTONIC, &start_time);
      printf("wrote this number: %u\n", ((uint32_t*)ctx)[(i*PAGE_SIZE)/4]);
	//printf("time to execute this op: %lf ns\n", ((double)end - start)/2.4);
	//printf("[app] sync op %u\n", i);
    }

    while(1) {
      ;
    }
    
    return 0;
}
