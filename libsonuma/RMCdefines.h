/*
 * Scale-Out NUMA Open Source License
 *
 * Copyright (c) 2017, Parallel Systems Architecture Lab, EPFL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the Parallel Systems Architecture Lab, EPFL,
 *   nor the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
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
 *  soNUMA library functions
 */

#ifndef H_RMC_DEFINES
#define H_RMC_DEFINES

#define MAX_NUM_WQ 64

#define KAL_REG_WQ      1
#define KAL_UNREG_WQ    6
#define KAL_REG_CQ      5
#define KAL_REG_CTX     3
#define KAL_PIN_BUFF    4
#define KAL_PIN         14

#define PAGE_SIZE 4096

typedef struct wq_entry {
  uint8_t op;
  volatile uint8_t SR;
  //set with a new WQ entry, unset when entry completed.
  //Required for pipelining async ops.
  volatile uint8_t valid;
  uint64_t buf_addr;
  uint64_t buf_offset;
  uint8_t cid;
  uint16_t nid;
  uint64_t offset;
  uint64_t length;
} wq_entry_t;

typedef struct cq_entry { 
  volatile uint8_t SR;
  volatile uint8_t tid;
} cq_entry_t;

typedef struct rmc_wq {
  wq_entry_t q[MAX_NUM_WQ];
  uint8_t head;
  volatile uint8_t SR;
} rmc_wq_t;

typedef struct rmc_cq {
  cq_entry_t q[MAX_NUM_WQ];
  uint8_t tail;
  volatile uint8_t SR;
} rmc_cq_t;

typedef struct qp_info {
  int node_cnt;
  int this_nid;
} qp_info_t;

#endif /* H_RMC_DEFINES */
