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
 *  All-software implementation of the RMC
 */

#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <assert.h>

#include <vector>
#include <algorithm>

#include "rmcd.h"

using namespace std;

//server information
static server_info_t *sinfo;

static volatile bool rmc_active;
static int fd;

//partitioned global address space - one entry per region
static char *ctx[MAX_NODE_CNT];

static int node_cnt, this_nid;

int alloc_wq(rmc_wq_t **qp_wq)
{
  int retcode, i;
  FILE *f;
  
  int shmid = shmget(IPC_PRIVATE, PAGE_SIZE,
			     SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[alloc_wq] shmget for WQ okay, shmid = %d\n",
	   shmid);
    *qp_wq = (rmc_wq_t *)shmat(shmid, NULL, 0);

    printf("[alloc_wq] shmat completed\n");


    f = fopen("wq_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[alloc_wq] shmget failed\n");
  }

  rmc_wq_t *wq = *qp_wq; 
  
  if (wq == NULL) {
    printf("[alloc_wq] Work Queue could not be allocated.");
    return -1;
  }
  
  //initialize wq memory
  printf("size of rmc_wq_t: %u\n", sizeof(rmc_wq_t));
  memset(wq, 0, sizeof(rmc_wq_t));

  printf("[alloc_wq] memset the WQ memory\n");
  
  retcode = mlock((void *)wq, PAGE_SIZE);
  if(retcode != 0) {
    DLog("[alloc_wq] WQueue mlock returned %d", retcode);
    return -1;
  } else {
    DLog("[alloc_wq] WQ was pinned successfully.");
  }

  //setup work queue
  wq->head = 0;
  wq->SR = 1;

  for(i=0; i<MAX_NUM_WQ; i++) {
    wq->q[i].SR = 0;
  }

  return 0;
}

int alloc_cq(rmc_cq_t **qp_cq)
{
  int retcode, i;
  FILE *f;
  
  int shmid = shmget(IPC_PRIVATE, PAGE_SIZE,
			     SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[alloc_cq] shmget for CQ okay, shmid = %d\n", shmid);
    *qp_cq = (rmc_cq_t *)shmat(shmid, NULL, 0);

    f = fopen("cq_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[alloc_cq] shmget failed\n");
  }

  rmc_cq_t *cq = *qp_cq; 
  
  if (cq == NULL) {
    DLog("[alloc_cq] Work Queue could not be allocated.");
    return -1;
  }
  
  //initialize cq memory
  memset(cq, 0, sizeof(rmc_cq_t));
    
  retcode = mlock((void *)cq, PAGE_SIZE);
  if(retcode != 0) {
    DLog("[alloc_cq] WQueue mlock returned %d", retcode);
    return -1;
  } else {
    DLog("[alloc_cq] WQ was pinned successfully.");
  }

  //setup work queue
  cq->tail = 0;
  cq->SR = 1;

  for(i=0; i<MAX_NUM_WQ; i++) {
    cq->q[i].SR = 0;
  }

  return 0;
}

int local_buf_alloc(char **mem)
{
  int retcode;
  FILE *f;
  
  int shmid = shmget(IPC_PRIVATE, PAGE_SIZE,
			     SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[local_buf_alloc] shmget for local buffer okay, shmid = %d\n",
	   shmid);
    *mem = (char *)shmat(shmid, NULL, 0);

    f = fopen("local_buf_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[local_buf_alloc] shmget failed\n");
  }

  if (*mem == NULL) {
    DLog("[local_buf_alloc] Local buffer could have not be allocated.");
    return -1;
  }
  
  memset(*mem, 0, PAGE_SIZE);
    
  retcode = mlock((void *)*mem, PAGE_SIZE);
  if(retcode != 0) {
    DLog("[local_buf_alloc] mlock returned %d", retcode);
    return -1;
  } else {
    DLog("[local_buf_alloc] was pinned successfully.");
  }

  return 0;
}

static int rmc_open(char *shm_name)
{   
  int fd;
  
  printf("[rmc_open] open called in VM mode\n");
  
  if ((fd=open(shm_name, O_RDWR|O_SYNC)) < 0) {
    return -1;
  }
  
  return fd;
}

static int soft_rmc_ctx_destroy()
{
  int i;
  
  ioctl_info_t info;
  
  info.op = RUNMAP;
  for(i=0; i<node_cnt; i++) {
    if(i != this_nid) {
      info.node_id = i;
      if(ioctl(fd, 0, (void *)&info) == -1) {
	printf("[soft_rmc_ctx_destroy] failed to unmap a remote region\n");
	return -1;
      }
    }
  }
  
  return 0;
}

static int net_init(int node_cnt, int this_nid, char *filename)
{
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int i = 0;
  char *pch;
  
  printf("[network] net_init <- \n");
  
  sinfo = (server_info_t *)malloc(node_cnt * sizeof(server_info_t));
  
  //retreive ID, IP, DOMID
  fp = fopen(filename, "r");
  if (fp == NULL)
    exit(EXIT_FAILURE);

  //get server information
  while ((read = getline(&line, &len, fp)) != -1) {
    pch = strtok (line,":");
    sinfo[i].nid = atoi(pch);
    printf("ID: %d ", sinfo[i].nid);
    pch = strtok(NULL, ":");
    strcpy(sinfo[i].ip, pch);
    printf("IP: %s ", sinfo[i].ip);
    pch = strtok(NULL, ":");
    sinfo[i].domid = atoi(pch);
    printf("DOMID: %d\n", sinfo[i].domid);
    i++;
  }

  printf("[network] net_init -> \n");
    
  return 0;
}

//allocates local memory and maps remote memory 
int ctx_map(char **mem, unsigned page_cnt)
{
  ioctl_info_t info; 
  int i;
    
  printf("[ctx_map] soft_rmc_alloc_ctx ->\n");
  unsigned long dom_region_size = page_cnt * PAGE_SIZE;
    
  ctx[this_nid] = *mem;
  
  printf("[ctx_map] registering remote memory, number of remote nodes %d\n", node_cnt-1);
  
  info.op = RMAP;

  //map the rest of pgas
  for(i=0; i<node_cnt; i++) {
    if(i != this_nid) {
      info.node_id = i;
      if(ioctl(fd, 0, (void *)&info) == -1) {
	printf("[ctx_map] ioctl failed\n");
	return -1;
      }
      
      printf("[ctx_map] mapping memory of node %d\n", i);
      
      ctx[i] = (char *)mmap(NULL, page_cnt * PAGE_SIZE,
			    PROT_READ | PROT_WRITE,
			    MAP_SHARED, fd, 0);
      if(ctx[i] == MAP_FAILED) {
	close(fd);
	perror("[ctx_map] error mmapping the file");
	exit(EXIT_FAILURE);
      }

#ifdef DEBUG_RMC
      //for testing purposes
      for(j=0; j<(dom_region_size)/sizeof(unsigned long); j++)
	printf("%lu\n", *((unsigned long *)ctx[i]+j));
#endif
    }
  }
  
  printf("[ctx_map] context successfully created, %lu bytes\n",
	 (unsigned long)page_cnt * PAGE_SIZE * node_cnt);
  
  //activate the RMC
  rmc_active = true;
  
  return 0;
}

int ctx_alloc_grant_map(char **mem, unsigned page_cnt)
{
  int i, srv_idx;
  int listen_fd;
  struct sockaddr_in servaddr; //listen
  struct sockaddr_in raddr; //connect, accept
  int optval = 1;
  unsigned n;
  FILE *f;
  
  printf("[ctx_alloc_grant_map] soft_rmc_connect <- \n");

  //allocate the pointer array for PGAS
  fd = rmc_open((char *)"/root/node");

  //first allocate memory
  unsigned long *ctxl;
  unsigned long dom_region_size = page_cnt * PAGE_SIZE;

  int shmid = shmget(IPC_PRIVATE, dom_region_size*sizeof(char), SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[ctx_alloc_grant_map] shmget okay, shmid = %d\n", shmid);
    *mem = (char *)shmat(shmid, NULL, 0);

    f = fopen("ctx_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[ctx_alloc_grant_map] shmget failed\n");
  }

  if(*mem != NULL) {
    printf("[ctx_alloc_grant_map] memory for the context allocated\n");
    memset(*mem, 0, dom_region_size);
    mlock(*mem, dom_region_size);
  }
  
  printf("[ctx_alloc_grant_map] managed to lock pages in memory\n");
  
  ctxl = (unsigned long *)*mem;

  //snovakov:need to do this to fault the pages into memory
  for(i=0; i<(dom_region_size*sizeof(char))/8; i++) {
    ctxl[i] = 0;
  }

  //register this memory with the kernel driver
  ioctl_info_t info;
  info.op = MR_ALLOC;
  info.ctx = (unsigned long)*mem;
  
  if(ioctl(fd, 0, &info) == -1) {
    perror("kal ioctl failed");
    return -1;
  }
  
  //initialize the network
  net_init(node_cnt, this_nid, (char *)"/root/servers.txt");
  
  //listen
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port=htons(PORT);
    
  if((setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) == -1) {
    printf("Error on setsockopt\n");
    exit(EXIT_FAILURE);
  }

  if(bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
    fprintf(stderr, "Address binding error\n");
    exit(EXIT_FAILURE);
  }

  if(listen(listen_fd, 1024) == -1) {
    printf("[ctx_alloc_grant_map] Listen call error\n");
    exit(EXIT_FAILURE);
  }

  for(i=0; i<node_cnt; i++) {
    if(i != this_nid) {
      if(i > this_nid) {
	printf("[ctx_alloc_grant_map] server accept..\n");
	char *remote_ip;
       
	socklen_t slen = sizeof(raddr);

       	sinfo[i].fd = accept(listen_fd, (struct sockaddr*)&raddr, &slen);

	//retrieve nid of the remote node
	remote_ip = inet_ntoa(raddr.sin_addr);
	
	printf("[ctx_alloc_grant_map] Connect received from %s, on port %d\n",
	       remote_ip, raddr.sin_port);

	//receive the reference to the remote memory
	while(1) {
	  n = recv(sinfo[i].fd, (char *)&srv_idx, sizeof(int), 0);
	  if(n == sizeof(int)) {
	    printf("[ctx_alloc_grant_map] received the node_id\n");
	    break;
	  }
	}

	printf("[ctx_alloc_grant_map] server ID is %u\n", srv_idx);
      } else {
	printf("[ctx_alloc_grant_map] server connect..\n");

	memset(&raddr, 0, sizeof(raddr));
	raddr.sin_family = AF_INET;
	inet_aton(sinfo[i].ip, &raddr.sin_addr);
	raddr.sin_port = htons(PORT);

	sinfo[i].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	while(1) {
	  if(connect(sinfo[i].fd, (struct sockaddr *)&raddr, sizeof(raddr)) == 0) {
	    printf("[ctx_alloc_grant_map] Connected to %s\n", sinfo[i].ip);
	    break;
	  }
	}
	unsigned n = send(sinfo[i].fd, (char *)&this_nid, sizeof(int), 0); //MSG_DONTWAIT
	if(n < sizeof(int)) {
	  printf("[ctx_alloc_grant_map] ERROR: couldn't send the node_id\n");
	  return -1;
	}

	srv_idx = i;
      }

      //first get the reference for this domain
      info.op = GETREF;
      info.node_id = srv_idx;
      info.domid = sinfo[srv_idx].domid;
      if(ioctl(fd, 0, (void *)&info) == -1) {
	printf("[ctx_alloc_grant_map] failed to unmap a remote region\n");
	return -1;
      }

      //send the reference to the local memory
      unsigned n = send(sinfo[srv_idx].fd, (char *)&info.desc_gref, sizeof(int), 0); //MSG_DONTWAIT
      if(n < sizeof(int)) {
	printf("[ctx_alloc_grant_map] ERROR: couldn't send the grant reference\n");
	return -1;
      }
      
      printf("[ctx_alloc_grant_map] grant reference sent: %u\n", info.desc_gref);

      //receive the reference to the remote memory
      while(1) {
	n = recv(sinfo[srv_idx].fd, (char *)&info.desc_gref, sizeof(int), 0);
	if(n == sizeof(int)) {
	  printf("[ctx_alloc_grant_map] received the grant reference\n");
	  break;
	}
      }
	
      printf("[ctx_alloc_grant_map] grant reference received: %u\n", info.desc_gref);
    
      //put the ref for this domain
      info.op = PUTREF;
      if(ioctl(fd, 0, (void *)&info) == -1) {
	printf("[ctx_alloc_grant_map] failed to unmap a remote region\n");
	return -1;
      }
    }    
  } //for

  //now memory map all the regions
  ctx_map(mem, page_cnt);
  
  return 0;
}

int main(int argc, char **argv)
{
  int i;
  
  //queue pairs
  volatile rmc_wq_t *wq = NULL;
  volatile rmc_cq_t *cq = NULL;
  
  //local context region
  char *local_mem_region;

  //local buffer
  char *local_buffer;

  if(argc != 3) {
    printf("[main] incorrect number of arguments\n");
    exit(1);
  }

  node_cnt = atoi(argv[1]);
  this_nid = atoi(argv[2]);
  
  //allocate a queue pair
  alloc_wq((rmc_wq_t **)&wq);
  alloc_cq((rmc_cq_t **)&cq);

  //create the global address space
  if(ctx_alloc_grant_map(&local_mem_region, MAX_REGION_PAGES) == -1) {
    printf("[main] context not allocated\n");
    return -1;
  }

  //allocate local buffer
  local_buf_alloc(&local_buffer);
  
  //WQ ptrs
  uint8_t local_wq_tail = 0;
  uint8_t local_wq_SR = 1;

  //CQ ptrs
  uint8_t local_cq_head = 0;
  uint8_t local_cq_SR = 1;
  
  uint8_t compl_idx;
  
  volatile wq_entry_t *curr;
  
#ifdef DEBUG_PERF_RMC
  struct timespec start_time, end_time;
  uint64_t start_time_ns, end_time_ns;
  vector<uint64_t> stimes;
#endif
  
  while(rmc_active) {
    while (wq->q[local_wq_tail].SR == local_wq_SR) {
#ifdef DEBUG_PERF_RMC
      clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
      
#ifdef DEBUG_RMC      
      printf("[main] reading remote memory, offset = %lu\n",
	     wq->q[local_wq_tail].offset);
      
      printf("[main] buffer address %lu\n",
	     wq->q[local_wq_tail].buf_addr);
      
      printf("[main] nid = %d; offset = %d, len = %d\n", wq->q[local_wq_tail].nid,
	     wq->q[local_wq_tail].offset, wq->q[local_wq_tail].length);
#endif
      curr = &(wq->q[local_wq_tail]);
      
      if(curr->op == 'r') {
	memcpy((uint8_t *)(local_buffer + curr->buf_offset),
	       ctx[curr->nid] + curr->offset,
	       curr->length);
      } else {
	memcpy(ctx[curr->nid] + curr->offset,
	       (uint8_t *)(local_buffer + curr->buf_offset),
	       curr->length);	
      }

#ifdef DEBUG_PERF_RMC
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      start_time_ns = BILLION * start_time.tv_sec + start_time.tv_nsec;
      end_time_ns = BILLION * end_time.tv_sec + end_time.tv_nsec;
      printf("[main] memcpy latency: %u ns\n", end_time_ns - start_time_ns);
#endif
      
#ifdef DEBUG_PERF_RMC
      clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
      
      compl_idx = local_wq_tail;
      
      local_wq_tail += 1;
      if (local_wq_tail >= MAX_NUM_WQ) {
	local_wq_tail = 0;
	local_wq_SR ^= 1;
      }
      
      //notify the application
      cq->q[local_cq_head].tid = compl_idx;
      cq->q[local_cq_head].SR = local_cq_SR;
      
      local_cq_head += 1;
      if(local_cq_head >= MAX_NUM_WQ) {
	local_cq_head = 0;
	local_cq_SR ^= 1;
      }

#ifdef DEBUG_PERF_RMC
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      start_time_ns = BILLION * start_time.tv_sec + start_time.tv_nsec;
      end_time_ns = BILLION * end_time.tv_sec + end_time.tv_nsec;
      printf("[main] notification latency: %u ns\n", end_time_ns - start_time_ns);
#endif
      
#ifdef DEBUG_RMC
      stimes.insert(stimes.begin(), end_time_ns - start_time_ns);
      
      if(stimes.size() == 100) {
	long sum = 0;
	sort(stimes.begin(), stimes.end());
	for(int i=0; i<100; i++)
	  sum += stimes[i];
	
	while (!stimes.empty())
	  stimes.pop_back();
      }
#endif
    }    
  }
  
  soft_rmc_ctx_destroy();
  
  printf("[main] RMC deactivated\n");
  
  return 0;
}

void deactivate_rmc()
{
  rmc_active = false;
}

