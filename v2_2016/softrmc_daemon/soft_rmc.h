/**
 * soNUMA core rmc implementation.
 *
 * Copyright (C) EPFL. All rights reserved.
 * @authors novakovic
 */

#ifndef SOFT_RMC_H
#define SOFT_RMC_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <arpa/inet.h>

#include "RMCdefines.h"

#define IP_LEN 16
#define PORT 7000

#define MAX_NODE_CNT 24
#define MAX_REGION_PAGES 4096 //7408

#define RMAP 1
#define RUNMAP 0
#define GETREF 2
#define PUTREF 3
#define MR_ALLOC 4

typedef struct server_info {
  int nid;
  char ip[IP_LEN];
  int domid;
  int fd;
} server_info_t;

typedef struct ioctl_info {
  int op;
  int node_id;
  int domid;
  int desc_gref;
  unsigned long ctx;
} ioctl_info_t;

typedef uintptr_t nam_version_t;

struct nam_obj_header {
  nam_version_t version;
};

//#define HW_OCC

#define OBJ_COUNT 2

//#define DEBUG_RMC

//RMC thread
/*
void deactivate_rmc();

//Configuration API
int soft_rmc_connect(int node_cnt, int this_nid);
int soft_rmc_wq_reg(rmc_wq_t *qp_wq);
int soft_rmc_cq_reg(rmc_cq_t *qp_cq);
int soft_rmc_ctx_alloc(char **mem, unsigned page_cnt);
*/
#endif
