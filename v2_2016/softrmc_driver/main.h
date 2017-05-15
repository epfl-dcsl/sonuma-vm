/*
 *  RMC
 *
 *  Installation and Usage instructions
 *
 *  Authors: 
 *  	Stanko Novakovic - EPFL (stanko.novakovic@epfl.ch)
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

#ifndef _MAIN_H_
#define _MAIN_H_

#include <xen/grant_table.h>

#define	MAX_MAC_NUM	10
#define MAX_RETRY_COUNT 5

#define ETH_P_TIDC			0x8888	
typedef struct timeval          timeval;
typedef struct list_head        list_head;
typedef struct page             page;
typedef struct ethhdr 		ethhdr;
typedef struct net_device 	net_device;
typedef struct packet_type 	packet_type;

#define RMC_MSG_TYPE_SESSION_DISCOVER 	77
#define RMC_MSG_TYPE_SESSION_DISCOVER_CONNECT 13 //new*
#define RMC_MSG_TYPE_SESSION_DISCOVER_ACK 	78
#define RMC_MSG_TYPE_CREATE_CHN		2
#define RMC_MSG_TYPE_CREATE_ACK 		4
#define RMC_MSG_TYPE_DESTROY_CHN 		8

#define RMC_ENTRY_ORDER 16//14//10//16//10//15

#define MAX_NUM_WQ 64//16
#define MAX_CTXS 16 //max number of registered regions is 16
#define MAX_PAGES 16384

#define BBUFF_SIZE 16

//TODO change this
#define MR_GREF(handle) (handle->desc_gref)
#define MR_EVT_PORT(handle) (handle->port)
#define MR_EVT_IRQ(handle) (handle->irq)

#define RMC_STATUS_INIT 	1
#define RMC_STATUS_LISTEN 	2
#define RMC_STATUS_CONNECTED 4
#define RMC_STATUS_SUSPEND   8

#define LINK_HDR 			sizeof(struct ethhdr)
#define MSGSIZE				sizeof(message_t)

#define MAX_REGION_PAGES 4096 //7408 //snovakov: this is max //65536 //16 //64
#define MAX_DESC_PAGES 256
#define MAX_ROOT_DESC_PAGES 16
#define MAX_NODE_CNT 24

//for ioctl
#define RMAP 1
#define RUNMAP 0
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

typedef struct message {
    u8		type;
    u8		mac_count;
    u8		mac[MAX_MAC_NUM][ETH_ALEN];
    domid_t 	nid;
    domid_t	guest_domids[MAX_MAC_NUM];	
    int		gref;
    int		remote_port;
} message_t;

typedef struct skb_queue{
    struct sk_buff *head;
    struct sk_buff *tail;
    int count;
} skb_queue_t;

struct channel_handle {
    int port;       
    int irq;        
};
typedef struct channel_handle channel_handle_t;

typedef struct Entry {
  int initialized;
  struct list_head mapping;
  u8 	mac[ETH_ALEN];
  domid_t domid;
  domid_t nid;

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
  struct page **pages;
} Entry;

#endif /* _MAIN_H_ */

