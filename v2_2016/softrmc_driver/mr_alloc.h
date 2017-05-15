/*
 *  XenLoop -- A High Performance Inter-VM Network Loopback 
 *
 *  Installation and Usage instructions
 *
 *  Authors: 
 *  	Jian Wang - Binghamton University (jianwang@cs.binghamton.edu)
 *  	Kartik Gopalan - Binghamton University (kartik@cs.binghamton.edu)
 *
 *  Copyright (C) 2007-2009 Kartik Gopalan, Jian Wang
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */



#ifndef _MR_ALLOC_H_
#define _MR_ALLOC_H_

#include <xen/xenbus.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include <xen/hypercall.h>
//epfl
//#include <xen/driver_util.h>
//#include <xen/gnttab.h>
#include <xen/grant_table.h>
#include <xen/balloon.h>
#include <xen/page.h>
#include <xen/events.h>
//

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

/* 
 * Shared REGION descriptor page 
 * 	sizeof(memory_region_t) should be no bigger than PAGE_SIZE
 */
struct memory_region {
    unsigned int num_pages; 
    void * region;
};
typedef struct memory_region memory_region_t;

mr_reference_info_t *create_mr_reference(unsigned long mem, domid_t remote_domid); //memory_region_t *mr, domid_t remote_domid);
void destroy_mr_reference(mr_reference_info_t *ref);
memory_region_t *mr_create(unsigned int page_count);
int mr_destroy(memory_region_t * mr);
bool mr_map(domid_t rdomid, int rgref, int rport, Entry *e, struct vm_area_struct *vma);
bool mr_unmap(Entry * e, unsigned long addr);
channel_handle_t *create_channel(domid_t rdomid);
void destroy_channel(channel_handle_t * handle);
void mr_init(void);
#endif
