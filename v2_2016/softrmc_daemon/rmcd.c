// Stanko Novakovic
// All-software implementation of RMC
// daemon

#include "soft_rmc.h"

#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <assert.h>

#include <vector>
#include <algorithm>

using namespace std;

#define BILLION 1000000000

//#define DEBUG_PERF_RMC

#ifndef DEBUG_RMC
#define DLog(M, ...)
#else
#define DLog(M, ...) fprintf(stdout, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

//server information
static server_info_t *sinfo;

static volatile bool rmc_active;
static int fd;

//pgas - one context supported
static char *ctx[MAX_NODE_CNT];

static int node_cnt, this_nid;

int alloc_wq(rmc_wq_t **qp_wq) {
  int retcode, i;
  FILE *f;
  
  int shmid = shmget(IPC_PRIVATE, PAGE_SIZE,
			     SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[soft_rmc_daemon] shmget for WQ okay, shmid = %d\n", shmid);
    *qp_wq = (rmc_wq_t *)shmat(shmid, NULL, 0);

    f = fopen("wq_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[soft_rmc_daemon] shmget failed\n");
  }

  rmc_wq_t *wq = *qp_wq; 
  
  if (wq == NULL) {
    DLog("[sonuma] Work Queue could not be allocated.");
    return -1;
  }
  
  //initialize the wq memory
  memset(wq, 0, sizeof(rmc_wq_t));
    
  retcode = mlock((void *)wq, PAGE_SIZE);
  if(retcode != 0) {
    DLog("[sonuma] WQueue mlock returned %d", retcode);
    return -1;
  } else {
    DLog("[sonuma] WQ was pinned successfully.");
  }

  //setup work queue
  wq->head = 0;
  wq->SR = 1;

  for(i=0; i<MAX_NUM_WQ; i++) {
    wq->q[i].SR = 0;
  }

  return 0;
}

int alloc_cq(rmc_cq_t **qp_cq) {
  int retcode, i;
  FILE *f;
  
  int shmid = shmget(IPC_PRIVATE, PAGE_SIZE,
			     SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[soft_rmc_daemon] shmget for CQ okay, shmid = %d\n", shmid);
    *qp_cq = (rmc_cq_t *)shmat(shmid, NULL, 0);

    f = fopen("cq_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[soft_rmc_daemon] shmget failed\n");
  }

  rmc_cq_t *cq = *qp_cq; 
  
  if (cq == NULL) {
    DLog("[sonuma] Work Queue could not be allocated.");
    return -1;
  }
  
  //initialize the wq memory
  memset(cq, 0, sizeof(rmc_cq_t));
    
  retcode = mlock((void *)cq, PAGE_SIZE);
  if(retcode != 0) {
    DLog("[sonuma] WQueue mlock returned %d", retcode);
    return -1;
  } else {
    DLog("[sonuma] WQ was pinned successfully.");
  }

  //setup work queue
  cq->tail = 0;
  cq->SR = 1;

  for(i=0; i<MAX_NUM_WQ; i++) {
    cq->q[i].SR = 0;
  }

  return 0;
}

int local_buf_alloc(char **mem) {
  int retcode;
  FILE *f;
  
  int shmid = shmget(IPC_PRIVATE, PAGE_SIZE,
			     SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[soft_rmc_daemon] shmget for local buffer okay, shmid = %d\n", shmid);
    *mem = (char *)shmat(shmid, NULL, 0);

    f = fopen("local_buf_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[soft_rmc_daemon] shmget failed\n");
  }

  if (*mem == NULL) {
    DLog("[sonuma] Local buffer could have not be allocated.");
    return -1;
  }
  
  //initialize the wq memory
  memset(*mem, 0, PAGE_SIZE);
    
  retcode = mlock((void *)*mem, PAGE_SIZE);
  if(retcode != 0) {
    DLog("[sonuma] WQueue mlock returned %d", retcode);
    return -1;
  } else {
    DLog("[sonuma] WQ was pinned successfully.");
  }

  return 0;
}

static int rmc_open(char *shm_name) {
    printf("[soft_rmc] open called in VM mode\n");
    
    int fd;
    
    if ((fd=open(shm_name, O_RDWR|O_SYNC)) < 0) {
        return -1;
    }
    
    return fd;
}

static int soft_rmc_ctx_destroy() {
    int i;

    ioctl_info_t info;

    info.op = RUNMAP;
    for(i=0; i<node_cnt; i++) {
	if(i != this_nid) {
	    info.node_id = i;
	    if(ioctl(fd, 0, (void *)&info) == -1) {
		printf("[soft_rmc] failed to unmap a remote region\n");
		return -1;
	    }
	    //munmap(ctx[i], dom_region_size);
	}
    }

    //close(fd);
    
    return 0;
}

static int net_init(int node_cnt, int this_nid, char *filename) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;

    printf("[network] net_init <- \n");
    
    sinfo = (server_info_t *)malloc(node_cnt * sizeof(server_info_t));

    //retreive ID, IP, DOMID
    fp = fopen(filename, "r");
    if (fp == NULL)
	exit(EXIT_FAILURE);

    // get server information
    while ((read = getline(&line, &len, fp)) != -1) {
      //printf("%s", line);
	char * pch = strtok (line,":");
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
int ctx_map(char **mem, unsigned page_cnt) {
    ioctl_info_t info; 
    int i;
    
    printf("[soft_rmc] soft_rmc_alloc_ctx ->\n");
    unsigned long dom_region_size = page_cnt * PAGE_SIZE;
    
    ctx[this_nid] = *mem;

    printf("[soft_rmc] registered local memory\n");
    printf("[soft_rmc] registering remote memory, number of remote nodes %d\n", node_cnt-1);

    info.op = RMAP;

    //map the rest of pgas
    for(i=0; i<node_cnt; i++) {
	if(i != this_nid) {
	    info.node_id = i;
	    if(ioctl(fd, 0, (void *)&info) == -1) {
		printf("[soft_rmc] ioctl failed\n");
		return -1;
	    }

	    printf("[soft_rmc] mapping memory of node %d\n", i);
	    
	    ctx[i] = (char *)mmap(NULL, page_cnt * PAGE_SIZE,
			    PROT_READ | PROT_WRITE,
			    MAP_SHARED, fd, 0);
	    if(ctx[i] == MAP_FAILED) {
		close(fd);
		perror("[soft_rmc] error mmapping the file");
		exit(EXIT_FAILURE);
	    }

#ifdef DEBUG_RMC
	    //for testing purposes
	    for(j=0; j<(dom_region_size)/sizeof(unsigned long); j++)
		printf("%lu\n", *((unsigned long *)ctx[i]+j));
#endif
	}
    }
    
    printf("[soft_rmc] context successfully created, %lu bytes\n",
	   (unsigned long)page_cnt * PAGE_SIZE * node_cnt);

    //activate the RMC
    rmc_active = true;
    
    return 0;
}

int ctx_alloc_grant_map(char **mem, unsigned page_cnt) {
  int i, srv_idx;
  int listen_fd; // errsv, errno;
  struct sockaddr_in servaddr; //listen
  struct sockaddr_in raddr; //connect, accept
  int optval = 1;
  unsigned n;
  FILE *f;
  
  printf("[soft_rmc] soft_rmc_connect <- \n");

  //allocate the pointer array for PGAS
  fd = rmc_open((char *)"/root/node");

  //first allocate memory
  unsigned long *ctxl;
  unsigned long dom_region_size = page_cnt * PAGE_SIZE;

  int shmid = shmget(IPC_PRIVATE, dom_region_size*sizeof(char), SHM_R | SHM_W);
  if(-1 != shmid) {
    printf("[soft_rmc] shmget okay, shmid = %d\n", shmid);
    *mem = (char *)shmat(shmid, NULL, 0);

    f = fopen("ctx_ref.txt", "w");
    fprintf(f, "%d", shmid);
    fclose(f);
  } else {
    printf("[soft_rmc] shmget failed\n");
  }

  if(*mem != NULL) {
    printf("[soft_rmc] memory for the context allocated\n");
    memset(*mem, 0, dom_region_size);
    mlock(*mem, dom_region_size);
  }
  
  printf("[soft_rmc] managed to lock pages in memory\n");
  
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
    //fprintf(stderr, "Listen call error: %s\n", strerror(errno));
    printf("[soft_rmc] Listen call error\n");
    exit(EXIT_FAILURE);
  }

  for(i=0; i<node_cnt; i++) {
    if(i != this_nid) {
      //first connect to it
      if(i > this_nid) {
	printf("[soft_rmc] server accept..\n");
	char *remote_ip;
       
	socklen_t slen = sizeof(raddr);

       	sinfo[i].fd = accept(listen_fd, (struct sockaddr*)&raddr, &slen);

	//retrieve nid of the remote node
	remote_ip = inet_ntoa(raddr.sin_addr); //retreive remote address
	
	printf("[soft_rmc] Connect received from %s\n", remote_ip);
	printf("[soft_rmc] Connect received on port %d\n", raddr.sin_port);
	//receive the reference to the remote memory
	while(1) {
	  n = recv(sinfo[i].fd, (char *)&srv_idx, sizeof(int), 0);
	  if(n == sizeof(int)) {
	    printf("[soft_rmc] received the node_id\n");
	    break;
	  }
	}

	printf("[soft_rmc] server ID is %u\n", srv_idx);
      } else {
	printf("[soft_rmc] server connect..\n");

	memset(&raddr, 0, sizeof(raddr));
	raddr.sin_family = AF_INET;
	inet_aton(sinfo[i].ip, &raddr.sin_addr);
	raddr.sin_port = htons(PORT);

	sinfo[i].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	while(1) {
	  if(connect(sinfo[i].fd, (struct sockaddr *)&raddr, sizeof(raddr)) == 0) {
	    printf("[soft_rmc] Connected to %s\n", sinfo[i].ip);
	    break;
	  }
	}
	unsigned n = send(sinfo[i].fd, (char *)&this_nid, sizeof(int), 0); //MSG_DONTWAIT
	if(n < sizeof(int)) {
	  printf("[soft_rmc] ERROR: couldn't send the node_id\n");
	  //return -1;
	}

	srv_idx = i;
      }

      //first get the reference for this domain
      info.op = GETREF;
      info.node_id = srv_idx;
      info.domid = sinfo[srv_idx].domid;
      if(ioctl(fd, 0, (void *)&info) == -1) {
	printf("[soft_rmc] failed to unmap a remote region\n");
	return -1;
      }

      //send the reference to the local memory
      unsigned n = send(sinfo[srv_idx].fd, (char *)&info.desc_gref, sizeof(int), 0); //MSG_DONTWAIT
      if(n < sizeof(int)) {
	printf("[soft_rmc] ERROR: couldn't send the grant reference\n");
	return -1;
      }
      
      printf("[soft_rmc] grant reference sent: %u\n", info.desc_gref);

      //receive the reference to the remote memory
      while(1) {
	n = recv(sinfo[srv_idx].fd, (char *)&info.desc_gref, sizeof(int), 0);
	if(n == sizeof(int)) {
	  printf("[soft_rmc] received the grant reference\n");
	  break;
	}
      }
	
      printf("[soft_rmc] grant reference received: %u\n", info.desc_gref);
    
      //put the ref for this domain
      info.op = PUTREF;
      if(ioctl(fd, 0, (void *)&info) == -1) {
	printf("[soft_rmc] failed to unmap a remote region\n");
	return -1;
      }
    }    
  } //for

  //now memory map all the regions
  ctx_map(mem, page_cnt);
  
  return 0;
}

const nam_version_t LOCKED_MASK = 1;
const nam_version_t UPDATING_MASK = 2;

inline bool version_is_updating(nam_version_t version) {
  return (version & UPDATING_MASK) != 0;
}

int main(int argc, char **argv) {
  int i;

  if(argc != 3) {
    printf("[rmcd] incorrect number of arguments\n");
    exit(1);
  }

  node_cnt = atoi(argv[1]);
  this_nid = atoi(argv[2]);

  //queue pairs
  volatile rmc_wq_t *wq = NULL;
  volatile rmc_cq_t *cq = NULL;
  
  //local context region
  char *local_mem_region;

  //local buffer
  char *local_buffer;
  
  //allocate a queue pair
  alloc_wq((rmc_wq_t **)&wq);
  alloc_cq((rmc_cq_t **)&cq);

  //create the global address space
  if(ctx_alloc_grant_map(&local_mem_region, MAX_REGION_PAGES) == -1) {
    printf("[soft_rmc_daemon] context not allocated\n");
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
  
  nam_obj_header *header_src;
  nam_obj_header *header_dst;
      
  printf("[soft_rmc] RMC activated\n");
  
  volatile wq_entry_t *curr;
  unsigned object_offset;
  
  int s, abort_cnt;

  unsigned long buf_cnt = 0;

  int consistent = 1;

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
      printf("[soft_rmc] reading remote memory, offset = %lu\n",
	     wq->q[local_wq_tail].offset);
      
      printf("[soft_rmc] buffer address %lu\n",
	     wq->q[local_wq_tail].buf_addr);
      
      printf("[soft_rmc] nid = %d; offset = %d, len = %d\n", wq->q[local_wq_tail].nid, wq->q[local_wq_tail].offset, wq->q[local_wq_tail].length);
#endif
      curr = &(wq->q[local_wq_tail]);
      
      //HW OCC implementation
#ifdef HW_OCC
      printf("[rmcd] this should not pop up\n");
      for(i = 0; i < OBJ_COUNT; i++) {
	//abort_cnt = 0;
	consistent = 1;
	while(consistent) {
	  object_offset = i * (curr->length/OBJ_COUNT);
	  memcpy((uint8_t *)(local_buffer + curr->buf_addr + object_offset),
		 ctx[curr->nid] + curr->offset + object_offset,
		 curr->length/OBJ_COUNT);
	  header_src = (nam_obj_header *)(ctx[curr->nid] + curr->offset + object_offset);
	  header_dst = (nam_obj_header *)(local_buffer + curr->buf_addr + object_offset);
	  if(header_src->version == header_dst->version) {
	    if(!version_is_updating(header_dst->version))
	      consistent = 0;
	  }
	}
      }
#else
      //old version w/o HW OCC
      if(curr->op == 'r') {
	memcpy((uint8_t *)(local_buffer + curr->buf_addr),
	       ctx[curr->nid] + curr->offset,
	       curr->length);
      } else {
	memcpy(ctx[curr->nid] + curr->offset,
	       (uint8_t *)(local_buffer + curr->buf_addr),
	       curr->length);	
      }

#ifdef DEBUG_PERF_RMC
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      start_time_ns = BILLION * start_time.tv_sec + start_time.tv_nsec;
      end_time_ns = BILLION * end_time.tv_sec + end_time.tv_nsec;
      printf("[rmcd] memcpy latency: %u ns\n", end_time_ns - start_time_ns);
#endif
      
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
      printf("[rmcd] notification latency: %u ns\n", end_time_ns - start_time_ns);
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
  
  printf("[soft_rmc] RMC deactivated\n");
  
  return 0;
}

void deactivate_rmc() {
  rmc_active = false;
}

