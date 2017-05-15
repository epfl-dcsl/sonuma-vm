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


#ifndef _MAPTABLE_H
#define _MAPTABLE_H

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/if_ether.h>

//#include "rmcfifo.h"

#define XENLOOP_ACK_TIMEOUT 5
#define DISCOVER_TIMEOUT 1

#define HASH_SIZE 64

ulong  hash(u8 *);
int    equal(void *, void *);

typedef struct Bucket {
	struct list_head bucket;
} Bucket;


typedef struct HashTable {
	ulong 		count,
			buckets; 
	Bucket  	table[HASH_SIZE];
  struct kmem_cache_t	*entries; //epfl
} HashTable;


#define check_descriptor(bfh) (bfh && bfh->msg_buff && bfh->msg_buff->descriptor)

//int init_hash_table(HashTable * ht, char * name);

#endif /* _MAPTABLE_H_*/ 
