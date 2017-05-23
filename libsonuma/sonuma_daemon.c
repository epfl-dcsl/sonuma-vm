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
 *  SoftRMC-specific extensions for libsonuma
 *
 *  Authors: 
 *  	Stanko Novakovic <stanko.novakovic@epfl.ch>
 */

#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "sonuma.h"

#define BILLION 1000000000

int kal_open(char *kal_name)
{  
  //nothing to do
  return 0;
}

int kal_reg_wq(int fd, rmc_wq_t **wq_ptr)
{
  int shmid;
  
  DLog("[kal_reg_wq] kal_reg_wq called.");
  
  FILE *f = fopen("wq_ref.txt", "r");
  fscanf(f, "%d", &shmid);
  printf("[kal_reg_wq] ID for the work queue is %d\n", shmid);
  *wq_ptr = (rmc_wq_t *)shmat(shmid, NULL, 0);
  if(*wq_ptr == NULL) {
    printf("[kal_reg_wq] shm attach failed (work queue)\n");
    return -1;
  }

  fclose(f);
  
  return 0;
}

int kal_reg_cq(int fd, rmc_cq_t **cq_ptr)
{
  int shmid;
  DLog("[kal_reg_cq] kal_reg_cq called.");
  
  FILE *f = fopen("cq_ref.txt", "r");
  fscanf(f, "%d", &shmid);
  printf("[kal_reg_cq] ID for the completion queue is %d\n", shmid);
  *cq_ptr = (rmc_cq_t *)shmat(shmid, NULL, 0);
  if(*cq_ptr == NULL) {
    printf("[kal_reg_cq] shm attach failed (completion queue)\n");
    return -1;
  }
  
  fclose(f);

  return 0;
}

int kal_reg_lbuff(int fd, uint8_t **buff_ptr, uint32_t num_pages)
{
  int shmid;
  FILE *f;
  
  if(*buff_ptr == NULL) {
    f = fopen("local_buf_ref.txt", "r");

    fscanf(f, "%d", &shmid);
    printf("[kal_reg_lbuff] ID for the local buffer is %d\n", shmid);

    *buff_ptr = (uint8_t *)shmat(shmid, NULL, 0);
    if(*buff_ptr == NULL) {
      printf("[kal_reg_lbuff] shm attach failed (local buffer)\n");
      return -1;
    }
    
    memset(*buff_ptr, 0, 4096);
  } else {
    printf("[kal_ref_lbuff] local buffer has been allocated, return\n");
    return -1;
  }

  fclose(f);
  
  return 0;
}

int kal_reg_ctx(int fd, uint8_t **ctx_ptr, uint32_t num_pages)
{
  int shmid;
  FILE *f;

  DLog("[kal_reg_ctx] kal_reg_ctx called.");
  
  if(*ctx_ptr == NULL) {
    f = fopen("ctx_ref.txt", "r");

    fscanf(f, "%d", &shmid);
    printf("[kal_reg_ctx] ID for the context memory is %d\n", shmid);

    *ctx_ptr = (uint8_t *)shmat(shmid, NULL, 0);    
    if(*ctx_ptr == NULL) {
      printf("[sonuma] shm attach failed (context)\n");
      return -1;
    }
    
    memset(*ctx_ptr, 0, 4096);
  } else {
    DLog("[kal_reg_ctx] error: context memory allready allocated\n");
    return -1;
  }

  fclose(f);
  
  return 0;
}
