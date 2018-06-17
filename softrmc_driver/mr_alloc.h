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

#ifndef _MR_ALLOC_H_
#define _MR_ALLOC_H_

#include <xen/xenbus.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/vmalloc.h> // MARK: compile on Centos ICC 6 clean image

#include <xen/hypercall.h>
#include <xen/grant_table.h>
#include <xen/balloon.h>
#include <xen/page.h>
#include <xen/events.h>

#include "debug.h"
#include "main.h"
  
typedef struct memory_region_reference {
  unsigned int num_pages;
  int desc_grefs[MAX_DESC_PAGES];
  int root_desc_gref;
} memory_region_reference_t;

typedef struct mr_reference_info {
  memory_region_reference_t *root_mr_ref;
  memory_region_reference_t *mr_refs[MAX_DESC_PAGES];
} mr_reference_info_t;

struct memory_region {
    unsigned int num_pages; 
    void *region;
};
typedef struct memory_region memory_region_t;

mr_reference_info_t *create_mr_reference(unsigned long mem, domid_t remote_domid);
void destroy_mr_reference(mr_reference_info_t *ref);
memory_region_t *mr_create(unsigned int page_count);
int mr_destroy(memory_region_t * mr);
bool mr_map(domid_t rdomid, int rgref, int rport, Entry *e, struct vm_area_struct *vma);
bool mr_unmap(Entry * e, unsigned long addr);
void mr_init(void);

#endif
