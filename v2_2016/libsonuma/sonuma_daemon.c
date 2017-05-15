/**
 * soNUMA library functions implementation.
 *
 * ustiugov: tested with flexus (solaris) but still incompatible with Linux
 *
 * Copyright (C) EPFL. All rights reserved.
 * @authors daglis, novakovic, ustiugov
 */

#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "sonuma.h"
//#include "sonuma_sr.h"

#define BILLION 1000000000

//////////////////////// LEGACY FROM FARM ///////////////////////////
/*
//the assumption is that we have just one peer
volatile uint64_t rsq_head = 0; //don't worry about this
uint64_t rsq_head_prev = 0;
volatile uint64_t local_cred_cnt = 0; //this is the latest snapshot that we compare to the current state
volatile uint8_t remote_cred_cnt = BBUFF_SIZE;
volatile uint64_t lsq_tail = 0;

uint64_t bubbles;
uint64_t non_bubbles;
uint64_t rdom_bubbles[16];
//uint64_t spin_cycles;

uint64_t sent_b = 0;

uint8_t old_len = 0;

uint32_t pkt_rcvd = 0;
uint32_t pkt_snt = 0;

uint32_t opc = 0;

inline uint64_t return_bubbles() {
return bubbles;
}

inline uint64_t return_non_bubbles() {
return non_bubbles;
}

inline void print_rdom_bubbles() {
int i;
for(i = 0; i<16; i++)
printf("[rmc runtime] rdom_bubbles[%d] = %lu\n", i, rdom_bubbles[i]);
}

inline uint64_t return_spin_cycles() {
return spin_cycles;
}
*/
/////////////////////// END OF LEGACY ///////////////////////////////


// global variable to switch Flexus to timing mode only once
int is_timing = 0;

/////////////////////// IMPLEMENTATION //////////////////////////////
int kal_open(char *kal_name) {
#ifdef FLEXUS
    DLog("[sonuma] kal_open called in FLEXUS mode. Do nothing.");
    return 0; // not used with Flexus
#else
    DLog("[sonuma] kal_open called in VM mode.");
    int fd;

    if ((fd=open(kal_name, O_RDWR|O_SYNC)) < 0) {
        return -1;
    }
    return fd;
#endif
}

int kal_reg_wq(int fd, rmc_wq_t **wq_ptr) {
  //int i, retcode;
    // ustiugov: WARNING: due to some Flexus caveats we need a whole page
    //*wq_ptr = (rmc_wq_t *)memalign(PAGE_SIZE, sizeof(rmc_wq_t));

    /*
    *wq_ptr = (rmc_wq_t *)memalign(PAGE_SIZE, PAGE_SIZE);

    //initialize the wq memory
    memset(*wq_ptr, 0, sizeof(rmc_wq_t));

    rmc_wq_t *wq = *wq_ptr;
    if (wq == NULL) {
        DLog("[sonuma] Work Queue could not be allocated.");
        return -1;
    }
    //retcode = mlock((void *)wq, sizeof(rmc_wq_t));
    retcode = mlock((void *)wq, PAGE_SIZE);
    if (retcode != 0) {
        DLog("[sonuma] WQueue mlock returned %d", retcode);
    } else {
        DLog("[sonuma] WQ was pinned successfully.");
    }

    //setup work queue
    wq->head = 0;
    wq->SR = 1;

    for(i=0; i<MAX_NUM_WQ; i++) {
        wq->q[i].SR = 0;
    }
    */
#ifdef FLEXUS
    DLog("[sonuma] Call Flexus magic call (WQUEUE).");
    call_magic_2_64((uint64_t)wq, WQUEUE, MAX_NUM_WQ);
#else
    DLog("[sonuma] kal_reg_wq called in VM mode.");
#ifdef KERNEL_RMC
    //posix_memalign((void **)wq, PAGE_SIZE, sizeof(rmc_wq_t));
    if(ioctl(fd, KAL_REG_WQ, (void *)wq) == -1) {
      return -1;
    }
#else
    FILE *f;
    int shmid;
    f = fopen("wq_ref.txt", "r");
    fscanf(f, "%d", &shmid);
    printf("[sonuma] ID for the work queue is %d\n", shmid);
    *wq_ptr = (rmc_wq_t *)shmat(shmid, NULL, 0);
    if(*wq_ptr == NULL) {
      printf("[sonuma] shm attach failed (work queue)\n");
    }

    fclose(f);
    //if(soft_rmc_wq_reg(*wq_ptr) < 0)
    //return -1;
#endif
#endif /* FLEXUS */

    return 0;
}

int kal_reg_cq(int fd, rmc_cq_t **cq_ptr) {
  //int i, retcode;
    // ustiugov: WARNING: due to some Flexus caveats we need a whole page
    //*cq_ptr = (rmc_cq_t *)memalign(PAGE_SIZE, sizeof(rmc_cq_t));
    /*
    *cq_ptr = (rmc_cq_t *)memalign(PAGE_SIZE, PAGE_SIZE);
    rmc_cq_t *cq = *cq_ptr;
    if (cq == NULL) {
        DLog("[sonuma] Completion Queue could not be allocated.");
        return -1;
    }
    //retcode = mlock((void *)cq, sizeof(rmc_cq_t));
    retcode = mlock((void *)cq, PAGE_SIZE);
    if (retcode != 0) {
        DLog("[sonuma] CQueue mlock returned %d", retcode);
    } else {
        DLog("[sonuma] CQ was pinned successfully.");
    }

    cq->tail = 0;
    cq->SR = 1;

    for(i=0; i<MAX_NUM_WQ; i++) {
        cq->q[i].SR = 0;
    }
    */
#ifdef FLEXUS
    DLog("[sonuma] Call Flexus magic call (CQUEUE).");
    call_magic_2_64((uint64_t)cq, CQUEUE, MAX_NUM_WQ);
#else
    DLog("[sonuma] kal_reg_cq called in VM mode.");
#ifdef KERNEL_RMC
    posix_memalign((void **)cq, PAGE_SIZE, sizeof(rmc_cq_t));
    //register completion queue
    if (ioctl(fd, KAL_REG_CQ, (void *)cq) == -1) {
        return -1;
    }
#else
    FILE *f;
    int shmid;
    f = fopen("cq_ref.txt", "r");
    fscanf(f, "%d", &shmid);
    printf("[sonuma] ID for the completion queue is %d\n", shmid);
    *cq_ptr = (rmc_cq_t *)shmat(shmid, NULL, 0);
    if(*cq_ptr == NULL) {
      printf("[sonuma] shm attach failed (completion queue)\n");
    }

    fclose(f);
    //if(soft_rmc_cq_reg(*cq_ptr) < 0)
    //return -1;
#endif
#endif /* FLEXUS */

    return 0;
}

int kal_reg_lbuff(int fd, uint8_t **buff_ptr, uint32_t num_pages) {
  //uint64_t buff_size = num_pages * PAGE_SIZE;
    if(*buff_ptr == NULL) {
	//buff hasn't been allocated in the main application code
	printf("[soft_rmc] LOCAL BUFFER memory HASN'T BEEN allocated.. allocating\n");
	fflush(stdout);
#ifdef FLEXUS
	uint8_t *buff = *buff_ptr;
	buff = memalign(PAGE_SIZE, buf_size*sizeof(uint8_t));
#else
	//*buff_ptr = (uint8_t *)malloc(buff_size * sizeof(uint8_t));
	//posix_memalign((void **)buff_ptr, PAGE_SIZE, buff_size*sizeof(char));
	FILE *f;
	int shmid;
	f = fopen("local_buf_ref.txt", "r");
	fscanf(f, "%d", &shmid);
	printf("[sonuma] ID for the local buffer is %d\n", shmid);
	*buff_ptr = (uint8_t *)shmat(shmid, NULL, 0);
	if(*buff_ptr == NULL) {
	  printf("[sonuma] shm attach failed (local buffer)\n");
	  return -1;
	}

	memset(*buff_ptr, 0, 4096);

	fclose(f);
#endif
	if (*buff_ptr == NULL) {
	    fprintf(stdout, "Local buffer could not be allocated.\n");
	    return -1;
	}
    }

#ifdef FLEXUS
    int i, retcode;
    // buffers allocation is done by app
    retcode = mlock((void *)buff, buff_size * sizeof(uint8_t));
    if (retcode != 0) {
        DLog("[sonuma] Local buffer %p mlock returned %d (buffer size = %"PRIu64" bytes)", *buff_ptr, retcode, buff_size);
    } else {
        DLog("[sonuma] Local buffer was pinned successfully.");
    }

    uint32_t counter = 0;
    //initialize the local buffer
    for(i = 0; i < (buff_size*sizeof(uint8_t)); i++) {
        buff[i] = 0;
        if ( (i % PAGE_SIZE) == 0) {
            counter = i*sizeof(uint8_t)/PAGE_SIZE;
            // map the buffer's pages in Flexus
            call_magic_2_64((uint64_t)&(buff[i] ), BUFFER, counter);
        }
    }

#ifdef FLEXI_MODE
    DLog("[sonuma] Call Flexus magic call (BUFFER_SIZE).");
    call_magic_2_64(42, BUFFER_SIZE, buff_size); // register local buffer
#endif /* FLEXI_MODE */

#else
#ifdef KERNEL_RMC
    DLog("[sonuma] kal_reg_lbuff called in VM mode.");
    //tell the KAL how long is the buffer
    ((int *)buff)[0] = num_pages;

    //pin buffer's page frames
    if(ioctl(fd, KAL_PIN_BUFF, buff) == -1)
    {
        return -1;
    }

    ((int *)buff)[0] = 0x0;
#else //SOFT_RMC
    //nothing to be done
#endif
#endif /* FLEXUS */

    return 0;
}

int kal_reg_ctx(int fd, uint8_t **ctx_ptr, uint32_t num_pages) {
    //assert(ctx_ptr != NULL);

#ifdef FLEXUS
    int i, retcode, counter;
    uint8_t *ctx = *ctx_ptr;
    int ctx_size = num_pages * PAGE_SIZE;
    if(ctx == NULL)
	ctx = memalign(PAGE_SIZE, ctx_size*sizeof(uint8_t));
    // buffers allocation is done by app
    retcode = mlock((void *)ctx, ctx_size*sizeof(uint8_t));
    if (retcode != 0) {
        DLog("[sonuma] Context buffer mlock returned %d", retcode);
    } else {
        DLog("[sonuma] Context buffer (size=%d, %d pages) was pinned successfully.", ctx_size, num_pages);
    }

    counter = 0;
    //initialize the context buffer
    ctx[0] = DEFAULT_CTX_VAL;
    call_magic_2_64((uint64_t)ctx, CONTEXTMAP, 0); // a single context #0 for each node now
    for(i = 0; i < (ctx_size*sizeof(uint8_t)); i++) {
        *(ctx + i) = DEFAULT_CTX_VAL;
        if ( (i % PAGE_SIZE) == 0) {
            // map the context's pages in Flexus
            call_magic_2_64((uint64_t)&(ctx[i]), CONTEXT, i);
        }
    }

#ifdef FLEXI_MODE
    DLog("[sonuma] Call Flexus magic call (CONTEXT_SIZE).");
    call_magic_2_64(42, CONTEXT_SIZE, ctx_size); // register ctx
#endif /* FLEXI_MODE */

#else // Linux, not flexus
    DLog("[sonuma] kal_reg_ctx called in VM mode.");
#ifdef KERNEL_RMC
    int tmp = ((int *)ctx)[0];

    ((int *)ctx)[0] = num_pages;

    //register context
    if (ioctl(fd, KAL_REG_CTX, ctx) == -1) {
        perror("kal ioctl failed");
        return -1;
    }

    ((int *)ctx)[0] = tmp;
#else //USER RMC (SOFT RMC)
    FILE *f;
    int shmid;

    if(*ctx_ptr == NULL) {
      f = fopen("ctx_ref.txt", "r");
      fscanf(f, "%d", &shmid);
      printf("[sonuma] ID for the context memory is %d\n", shmid);
      *ctx_ptr = (uint8_t *)shmat(shmid, NULL, 0);

      if(*ctx_ptr == NULL) {
	printf("[sonuma] shm attach failed (context)\n");
	return -1;
      }

      memset(*ctx_ptr, 0, 4096);

      fclose(f);

      //if(soft_rmc_ctx_alloc((char **)ctx_ptr, num_pages) < 0)
      //    return -1;
    } else {
	DLog("[sonuma] error: context memory allready allocated\n");
	return -1;
    }
#endif
#endif /* FLEXUS */

    return 0;
}

void flexus_signal_all_set() {
#ifdef FLEXUS
  if (is_timing == 0) {
#ifdef DEBUG_FLEXUS_STATS
        // global variables for sonuma operation counters
        op_count_issued = 0;
        op_count_completed = 0;
#endif

        DLog("[sonuma] Call Flexus magic call (ALL_SET).");
        call_magic_2_64(1, ALL_SET, 1);
        is_timing = 1;
    } else {
        DLog("[sonuma] (ALL_SET) magic call won't be called more than once.");
    }
#else
  DLog("[sonuma] flexus_signal_all_set called in VM mode. Do nothing.");
    // otherwise do nothing
#endif /* FLEXUS */
    }

static __inline__ unsigned long long rdtsc(void) {
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}

  /*
    unsigned long long start_tsc, stop_tsc;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (start_tsc));
  */
    /*
  struct timespec start_time, end_time;
  uint64_t start_time_ns, end_time_ns;

  clock_gettime(CLOCK_MONOTONIC, &start_time);
    */

void rmc_rread_sync(rmc_wq_t *wq, rmc_cq_t *cq, uint64_t lbuff_slot, int snid, uint32_t ctx_id, uint64_t ctx_offset, uint64_t length) {

    uint8_t wq_head = wq->head;
    uint8_t cq_tail = cq->tail;

#ifdef FLEXUS
    DLogPerf("[sonuma] rmc_rread_sync called in Flexus mode.");

    PASS2FLEXUS_STATS(wq_head, NEWWQENTRY_START, 0);

    DLogPerf("lbuff_slot: %"PRIu64" snid: %u ctx_id: %lu ctx_offset %"PRIu64" length: %"PRIu64, lbuff_slot, snid, ctx_id, ctx_offset, length);

    length = length / BLOCK_SIZE; // number of cache lines
    ctx_offset >>= CACHE_BLOCK_BITS;
    create_wq_entry(RMC_READ, wq->SR, (uint8_t)ctx_id, (uint16_t)snid, lbuff_slot, ctx_offset, length, (uint64_t)&(wq->q[wq_head]));

    PASS2FLEXUS_STATS(wq_head, NEWWQENTRY, 0);

#else // Linux below
    DLogPerf("[sonuma] rmc_rread_sync called in VM mode.");
    //create_wq_entry_emu(wq, lbuff_slot, snid, ctx_id, ctx_offset, length);

    wq->q[wq_head].buf_addr = lbuff_slot;
    wq->q[wq_head].cid = ctx_id;
    wq->q[wq_head].offset = ctx_offset;
    if(length < 64)
        wq->q[wq_head].length = 64; //at least 64B
    else
        wq->q[wq_head].length = length; //specify the length of the transfer
    wq->q[wq_head].op = 'r';
    wq->q[wq_head].nid = snid;
    //soNUMA v2.1
    wq->q[wq_head].valid = 1;
    wq->q[wq_head].SR = wq->SR;
#endif

    //printf("[sonuma_lib] WQ used %u\n", wq->head);

    wq->head =  wq->head + 1;
    // check if WQ reached its end
    if (wq->head >= MAX_NUM_WQ) {
        wq->head = 0;
        wq->SR ^= 1;
    }

    // wait for a completion of the entry
    while(cq->q[cq_tail].SR != cq->SR) {
    }

    // mark the entry as invalid, i.e. completed
    wq->q[cq->q[cq_tail].tid].valid = 0;

#ifdef FLEXUS
    PASS2FLEXUS_STATS(cq_tail, WQENTRYDONE, 0);
#endif

    //printf("[sonuma_lib] CQ used %u\n", cq->tail);
    cq->tail = cq->tail + 1;

    // check if WQ reached its end
    if (cq->tail >= MAX_NUM_WQ) {
        cq->tail = 0;
        cq->SR ^= 1;
    }
}

    /*
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (stop_tsc));
    printf("[uB] stop_tsc - start_tsc = %f\n", (float)(stop_tsc - start_tsc)/2.4);
    */
    /*
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    start_time_ns = BILLION * start_time.tv_sec + start_time.tv_nsec;
    end_time_ns = BILLION * end_time.tv_sec + end_time.tv_nsec;
    printf("[rmcd] read latency: %u ns\n", end_time_ns - start_time_ns);
    */

/*
int rmc_init(int node_cnt, int this_nid) {
#ifndef FLEXUS
    qp_info_t * qp_info = (qp_info_t *)malloc(sizeof(qp_info_t));

    soft_rmc_connect(node_cnt, this_nid); //Initialize PGAS

    qp_info->node_cnt = node_cnt;
    qp_info->this_nid = this_nid;

    printf("[sonuma] activating RMC..\n");
    return pthread_create(&rmc_thread,
			  NULL,
			  core_rmc_fun,
			  (void *)qp_info);
#else //FLEXUS
    return 0;
#endif
}

void rmc_deinit() {
#ifndef FLEXUS
    deactivate_rmc();
    pthread_join(rmc_thread, NULL);
#else

#endif
}
*/

//*********************** MESSAGING (SEND/RECV) ******************************//


//#define BBUFF_SIZE 128

//for messaging
//the assumption is that we have just one peer
volatile uint64_t rsq_head = 0;
uint64_t rsq_head_prev = 0;

//this is the latest snapshot that we compare to the current state
volatile uint64_t local_cred_cnt = 0;
volatile uint8_t remote_cred_cnt = BBUFF_SIZE;
volatile uint64_t lsq_tail = 0;

union sq_pl {
  uint32_t ctx_offset;
  uint32_t len;
  char data[PL_SIZE];
};

typedef struct sq_entry {
  //char data[PL_SIZE];
  union sq_pl pl;
  volatile uint8_t len;
  volatile int8_t cred; //local credit
  uint8_t flag;
  volatile uint8_t valid;
} sq_entry_t;


static inline int rmc_recv_replenish(char *ctx) {
  sq_entry_t *sq_entry = (sq_entry_t *)(ctx + ((lsq_tail % BBUFF_SIZE) * BLOCK_SIZE));
  //int stall_cnt = 0;

  DLog("[rmc_recv_replenish] start\n");
  DLog("[rmc_recv_replenish] waiting for credits..\n");
  DLog("[rmc_recv_replenish] local tail = %lu\n", lsq_tail);

  while(sq_entry->valid == 0)
    ;

  DLog("[rmc_recv_replenish] sq entry available..\n");

  if(sq_entry->valid > 0) {
    remote_cred_cnt += sq_entry->cred;

    DLog("[rmc_recv_replenish] credits replenished; available amount = %d\n", remote_cred_cnt);

    DLog("[rmc_recv_replenish] len of this message is %d\n", sq_entry->len);

    lsq_tail += 1;
    sq_entry->valid = 0; //invalidate entry

    return 0;
  }

  return -1;
}

int rmc_send(rmc_wq_t *wq, rmc_cq_t *cq, char *ctx, char *lbuff_ptr,
	     int lbuff_offset, char *data, int size, int snid) {
  int i = 0, n = 0;
  volatile sq_entry_t *sq_entry;
  uint64_t roffset, msg_length;

  //uint8_t tid;
  //uint8_t wq_head = wq->head % MAX_NUM_WQ;
  //uint8_t cq_tail = cq->tail % MAX_NUM_WQ;
  assert(ctx != NULL);
  assert(lbuff_ptr != NULL);
  
  uint32_t bytes_left = size;

  //how many packets we need to send as part of this transfer
  uint8_t pkt_num = (size + PL_SIZE) / PL_SIZE;

#ifdef DEBUG
  DLog("[rmc_send] number of packets to send = %d\n", pkt_num);
  assert(rsq_head >= rsq_head_prev);
  rsq_head_prev = rsq_head;
#endif

  if(remote_cred_cnt == 0) { //if credit == 0, wait
    DLog("[rmc_send] Credit count is zero. Waiting for credits...\n");
    rmc_recv_replenish(ctx);
  }

  if(size < RW_THR) { //use rwrite to send data
    DLog("[rmc_send] message is smaller than threshold of %d bytes. Using RWRITE.\n", RW_THR);
    //check if there is enough credit/slots on the receiver side
    if(pkt_num > remote_cred_cnt)
      pkt_num = remote_cred_cnt; //send as much as you can

    //check if wrap around is needed
    if(BBUFF_SIZE - (rsq_head % BBUFF_SIZE) <= pkt_num) {
      pkt_num = BBUFF_SIZE - (rsq_head % BBUFF_SIZE);
    }
    DLog("[rmc_send] actual number of packets that can be sent = %d\n", pkt_num);

    for(;i<pkt_num;i++) { //packetize
      sq_entry = (sq_entry_t *)(lbuff_ptr + (i*BLOCK_SIZE));

      sq_entry->valid = 1;
      sq_entry->cred = (lsq_tail - local_cred_cnt);
      local_cred_cnt = lsq_tail; //take a snapshot of the current tail

      //sq_entry->head = (uint8_t)(rsq_head + i + 1);
      sq_entry->flag = RMC_WRITE;

      if(bytes_left > PL_SIZE) {
        memcpy((char*)sq_entry->pl.data, data+(i*PL_SIZE), PL_SIZE);
        sq_entry->len = PL_SIZE;
        DLog("[rmc_send] sq_entry->len = %d, pkt num = %d, pkt_num = %d, sq_entry->head = %d\n", sq_entry->len, i, pkt_num, sq_entry->flag);
        n += PL_SIZE;
      }
      else {
        memcpy((char*)sq_entry->pl.data, data+i*PL_SIZE, bytes_left);
        sq_entry->len = bytes_left;
        DLog("[rmc_send] sq_entry->len = %d, pkt num = %d, pkt_num = %d\n", sq_entry->len, i, pkt_num);
        n += bytes_left;
      }
      //sq_entry->valid = 1;

      bytes_left -= PL_SIZE;
    }
  }
  else { //use rread to send data
    DLog("[rmc_send] message is larger than threshold of %d bytes. Using RREAD.\n", RW_THR);
    pkt_num = 1; //send just one packet
    sq_entry = (sq_entry_t *)lbuff_ptr;
    sq_entry->valid = 1;
    sq_entry->cred = (lsq_tail - local_cred_cnt);
    local_cred_cnt = lsq_tail; //take a snapshot of the current tail
    sq_entry->flag = RMC_READ;
    //memcpy(ctx+(BBUFF_SIZE*64), data, size); //avoid copying the data to the context for now (second page)
    sq_entry->pl.ctx_offset = BBUFF_SIZE*BLOCK_SIZE;
    sq_entry->pl.len = size;
    n += size;
  }

  if(pkt_num > 0) {
    //sync up first
    //rmc_wait(wq, cq, NULL, 0);

    //schedule a remote write
    roffset = (rsq_head % BBUFF_SIZE) * BLOCK_SIZE;
    msg_length = pkt_num * sizeof(sq_entry_t);
    //MICRO: wait not necessary, passing SR_CTX_ID

    DLog("[rmc_send] Sending %d bytes from local buffer, starting from address 0x%lx\n", msg_length, lbuff_ptr);
    //rmc_rwrite(wq, cq, (uint64_t)lbuff_ptr, snid,
    rmc_rwrite_sync(wq, cq, (uint64_t)lbuff_offset, snid,
         SR_CTX_ID, roffset, msg_length);

    //move the head and update the credit count
    rsq_head += pkt_num;
    remote_cred_cnt -= pkt_num; //this is going to be updated by the receiver at some point

    //wait for the receiver to read all the data/replenish credit
    if(size >= RW_THR)
      rmc_recv_replenish(ctx);

    DLog("[rmc_send] number of credits on the remote node is %d\n", remote_cred_cnt);
    DLog("[rmc_send] current head is %lu\n", rsq_head);
  }

  return n;
}

static inline void rmc_send_replenish(rmc_wq_t *wq, rmc_cq_t *cq, volatile char *ctx, char *lbuff_ptr,
				      int lbuff_offset, int snid) {
  //uint8_t wq_head = wq->head % MAX_NUM_WQ;
  //uint8_t cq_tail = cq->tail % MAX_NUM_WQ;
  //uint8_t tid;

  sq_entry_t *sq_entry = (sq_entry_t *)lbuff_ptr;
  uint64_t roffset, msg_length;

  DLog("[rmc_send_replenish] start\n");
  DLog("[rmc_send_replenish] replenishing credits.. credits available = %lu\n",
   lsq_tail - local_cred_cnt);
  DLog("[rmc_send_replenish] replenishing credits.. remote head = %lu\n", rsq_head);

  sq_entry->valid = 1;
  sq_entry->len = 0;
  sq_entry->cred = (lsq_tail - local_cred_cnt);
  local_cred_cnt = lsq_tail; //take a snapshot of the current tail

  //schedule a remote write
  roffset = (rsq_head % BBUFF_SIZE) * BLOCK_SIZE;
  msg_length = sizeof(sq_entry_t);

  //sync-up first
  //rmc_wait(wq, cq, NULL, 0);
  //MICRO: wait not necessary, passing SR_CTX_ID
  //rmc_rwrite(wq, cq, (uint64_t)lbuff_ptr, snid,
  rmc_rwrite_sync(wq, cq, (uint64_t)0, snid,
       SR_CTX_ID, roffset, msg_length);

  rsq_head += 1;
  remote_cred_cnt -= 1; //this is going to be updated by the receiver at some point
}

int rmc_recv(rmc_wq_t *wq, rmc_cq_t *cq, char *ctx, char *lbuff_ptr,
	     int lbuff_offset, int snid, char *data, int size) {
  //uint8_t wq_head = wq->head % MAX_NUM_WQ;
  //uint8_t cq_tail = cq->tail % MAX_NUM_WQ;
  //uint8_t tid;

  sq_entry_t *sq_entry = (sq_entry_t *)(ctx + ((lsq_tail % BBUFF_SIZE) * BLOCK_SIZE));
  char *data_ptr = data;
  int n = 0;
  //int pkt_num = 0;
  uint8_t pkt_num = (size + PL_SIZE) / PL_SIZE;
  
  DLog("[rmc_recv] sq_entry->len = %d; sq_entry->valid = %d; sq_entry->cred = %d; sq_entry->head = %d.\n\tSpinning on address %lx\n",
        sq_entry->len, sq_entry->valid, sq_entry->cred, sq_entry->flag, ctx);
  //wait for a packet
  while(sq_entry->valid == 0)
    ;

  //while(sq_entry->valid > 0) {
  while(pkt_num > 0) {
    DLog("[rmc_recv] sq_entry->len = %d; sq_entry->valid = %d; sq_entry->cred = %d; sq_entry->head = %d\n", sq_entry->len, sq_entry->valid, sq_entry->cred, sq_entry->flag);

    remote_cred_cnt += sq_entry->cred;

    DLog("[rmc_recv] credits replenished; available amount = %d\n", remote_cred_cnt);

    if(sq_entry->flag == RMC_WRITE) {
      memcpy(data_ptr, sq_entry->pl.data, sq_entry->len); //copy only valid bytes

      data_ptr += sq_entry->len;
      n += sq_entry->len;

      sq_entry->len = 0; //delete len
      sq_entry->valid = 0; //invalidate entry
      memset(sq_entry->pl.data, 0, sq_entry->len);
      
      lsq_tail += 1;

      //one message per call at most
      /*
#ifdef DEBUG
      assert(n <= size);
#endif
      */
      //if(n >= size)
      //break;

      sq_entry = (sq_entry_t *)(ctx + ((lsq_tail % BBUFF_SIZE) * BLOCK_SIZE));
      printf("received packet %u\n", pkt_num);	     
      pkt_num--;
    }
    else { //need to issue a bulk remote read
      //make sure you wait for the wq_head slot available (we have async and sync operations interleaved here)
      //rmc_wait(wq, cq, NULL, 0);
      //MICRO: not sure rmc_wait (above) is necessary anymore.
      //passing SR_CTX_ID to rread_sync
      rmc_rread_sync(wq, cq, (uint64_t)lbuff_ptr, snid, SR_CTX_ID,
         sq_entry->pl.ctx_offset,
         sq_entry->pl.len); //need to send the len as well

      memcpy(data_ptr, lbuff_ptr, sq_entry->pl.len); //copy the data to the application buffer

      n += sq_entry->pl.len;
      sq_entry->len = 0;
      sq_entry->valid = 0;
      lsq_tail += 1;
      //replenish credits and let the receiver know that the transfer has completed
      rmc_send_replenish(wq, cq, ctx, lbuff_ptr, lbuff_offset, snid);
      break;
    }
  }

  DLog("[rmc_recv] old tail = %lu, current tail = %lu\n", local_cred_cnt, lsq_tail);

  if(lsq_tail - local_cred_cnt >= (BBUFF_SIZE/2)) {
    rmc_send_replenish(wq, cq, ctx, lbuff_ptr, lbuff_offset, snid);
  }

  return n;
}
