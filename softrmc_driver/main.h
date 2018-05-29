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
 *  Remote memory driver for Scale-Out NUMA
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <xen/grant_table.h>

#define MAX_REGION_PAGES 4096 
#define MAX_DESC_PAGES 256
#define MAX_ROOT_DESC_PAGES 16

#define MAX_NODE_CNT 16

#define RUNMAP 0
#define RMAP 1
#define GETREF 2
#define PUTREF 3
#define MR_ALLOC 4

typedef struct ioctl_info {
  int op;
  int node_id;
  int domid;
  int desc_gref;
  unsigned long ctx;
} ioctl_info_t;

typedef struct Entry {
  domid_t domid;
  domid_t nid;

  //vm structs
  void *mr_info;
  struct vm_struct *root_rdescriptor_vmarea;
  struct vm_struct *rdescriptor_vmarea[MAX_DESC_PAGES];
  grant_handle_t dhandle;
  int page_cnt;
  
  //reference to remote memory's descriptor
  int gref;
  
  //mapped area
  int flags;
  struct vm_area_struct * vma;
  grant_handle_t fhandles[MAX_REGION_PAGES];
  struct gnttab_map_grant_ref map_ops[MAX_REGION_PAGES];
  struct gnttab_unmap_grant_ref unmap_ops[MAX_REGION_PAGES];
  struct gnttab_map_grant_ref kmap_ops[MAX_REGION_PAGES];
  struct gnttab_unmap_grant_ref kunmap_ops[MAX_REGION_PAGES]; // MARK
  struct page **pages;
} Entry;

#endif /* _MAIN_H_ */

