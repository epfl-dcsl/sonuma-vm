/*
 *  Remote memory driver for Scale-Out NUMA
 *
 *  Authors: 
 *  	Stanko Novakovic - EPFL (stanko.novakovic@epfl.ch)
 *
 *  Copyright (C) 2015 - Stanko Novakovic
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/genhd.h>

#include <xen/hypercall.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>
#include <xen/evtchn.h>
#include <xen/grant_table.h>
#include <xen/xen-ops.h>
#include <xen/interface/grant_table.h>
#include <xen/interface/io/ring.h>
#include <xen/interface/xen.h>
#include <asm/xen/page.h>

#include <net/sock.h>
#include <net/inet_sock.h>
#include <net/protocol.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <net/neighbour.h>
#include <net/dst.h>
#include <linux/if_ether.h>
#include <net/inet_common.h>
#include <linux/inetdevice.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/genhd.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include <time.h>

#include "main.h"
#include "debug.h"
#include "maptable.h"
#include "mr_alloc.h"

//memory region device
static dev_t mmap_dev;
static struct cdev mmap_cdev;

//my NID
static int mynid = 0;
module_param(mynid, int, 0);

unsigned long apps_mem;

//number of pages to allocate/map
static int page_cnt_log2 = 4;
module_param(page_cnt_log2, int, 0);

static int shm_open(struct inode *inode, struct file *filp);
static int shm_release(struct inode *inode, struct file *filp);
static int shm_mmap(struct file *filp, struct vm_area_struct *vma);
static long shm_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

//all sop_shmmap's methods
static struct file_operations shmmap_fops = {
    .open = shm_open,
    .release = shm_release,
    .mmap = shm_mmap,
    .unlocked_ioctl = shm_ioctl,
    .owner = THIS_MODULE,
};

//domain ids of remote domains -- support up to 16 domains
domid_t rdomids[MAX_NODE_CNT];
Entry rdomh[MAX_NODE_CNT];
unsigned rdom_connected[MAX_NODE_CNT];
int num_of_doms = 1;

//this node's dom id
static domid_t mydomid;

//local memory region
memory_region_t *local_mr;

//current entry being used for mmap operation
Entry *current_entry;

static domid_t get_my_domid(void) {
  char *domidstr;
  domid_t domid;

  domidstr = xenbus_read(XBT_NIL, "domid", "", NULL);
  if ( IS_ERR(domidstr) ) {
    EPRINTK("xenbus_read error\n");
    return PTR_ERR(domidstr);
  }

  domid = (domid_t) simple_strtoul(domidstr, NULL, 10);

  kfree(domidstr);

  return domid;
}

static int write_xenstore(int status) {
  int err = 1;

  printk(KERN_CRIT "write_xenstore\n");

  err = xenbus_printf(XBT_NIL, "xenloop", "xenloop", "%d", status);
  if (err) {
      printk(KERN_CRIT "writing xenstore xenloop status failed, err = %d \n", err);
  }

  return err;
}

//helper functions for dom/node search
static inline Entry* dom_lookup(domid_t rdomid) {
    uint8_t i = 0;
    uint8_t ret = 0;

    printk(KERN_CRIT "dom_lookup ->\n");
    printk(KERN_CRIT "rdomid in message is %d\n", rdomid);
    
    for(; i<num_of_doms; i++) {
	printk(KERN_CRIT "[sonuma_drv] dom_lookup: node %d\n", i);
	if(rdomid == rdomh[i].domid) {
	    printk(KERN_CRIT "[sonuma_drv] dom_lookup: found node with domid %d\n",
		   rdomh[i].domid);
	    ret = i; 
	    break;
	}
    }

    return &rdomh[ret];
}

static inline Entry *node_lookup(int nid) {
    uint8_t i = 0;
    
    for(; i<num_of_doms; i++) {
	printk(KERN_CRIT "[sonuma_drv] node_lookup: nid = %d, rdomh..nid = %d\n", nid, rdomh[i].nid);
	if(nid == rdomh[i].nid) {
	    printk(KERN_CRIT "[sonuma_drv] node ID found %d\n", rdomh[i].nid);
	    return &rdomh[i];
	}
    }

    return NULL;
}

inline unsigned dom_is_connected(domid_t rdomid) {
    uint8_t i = 0;

    for(; i<num_of_doms; i++) {
	if(rdomid == rdomids[i]) {
	    if(rdom_connected[i] == 1) {
		return 1;
	    } else {
		printk(KERN_CRIT "[dom_is_connected] dom is not connected ..\n");
		rdom_connected[i] = 1;
		return 0;
	    }
	}
    }
    
    return 1;
}

//packets exchanged between discovery and nodes, and between nodes
/*
static packet_type rmc_ptype = {
    .type = __constant_htons(ETH_P_TIDC),
    .func = session_recv,
    .dev = NULL,
    .af_packet_priv = NULL,
};
*/
int net_init(void) {
    int ret = 0;
    
    TRACE_ENTRY;
    
    printk(KERN_CRIT "net_init <-\n");

    //register session update handler -- information about collocated nodes
    //dev_add_pack(&rmc_ptype);

    TRACE_EXIT;
    return ret;
}

void net_exit(void) {
    printk(KERN_CRIT "net_exit <-\n");
    //dev_remove_pack(&rmc_ptype);
}

static long shm_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    Entry *e;    
    ioctl_info_t info;
    //memory_region_reference_t *region_ref;
    mr_reference_info_t *mr_info;
    
    if (copy_from_user(&info, (unsigned char *)arg, sizeof(ioctl_info_t)))
	return -EFAULT;

    printk(KERN_CRIT "[sonuma_drv] info.op = %d, info.node_id = %d\n", info.op, info.node_id);

    //requesting grant reference
    if(info.op == MR_ALLOC) {
      apps_mem = info.ctx;
    } else if(info.op == GETREF) {
      printk(KERN_CRIT "[ioctl] Operation: GETREF for remote domain = %d\n", info.node_id);
      printk(KERN_CRIT "[ioctl] initializing remote domain\n");
      
      rdomh[info.node_id].nid = info.node_id;
      rdomh[info.node_id].domid = info.domid;
      
      mr_info = create_mr_reference(apps_mem, rdomh[info.node_id].domid);
      printk(KERN_CRIT "[ioctl] reference created\n");
      
      //save the ref for unallocation
      rdomh[info.node_id].mr_info = (void *)mr_info;
      num_of_doms++;

      if(mr_info != NULL) {
	info.desc_gref = mr_info->root_mr_ref->root_desc_gref;      
	printk(KERN_CRIT "[ioctl] Reference ID is %u\n", info.desc_gref);      
	if (copy_to_user((unsigned char *)arg, &info, sizeof(ioctl_info_t)))
	  return -EFAULT;
      }

    } else if(info.op == PUTREF) {
      printk(KERN_CRIT "[ioctl] Operation: PUTREF for remote domain = %d\n", info.node_id);
      rdomh[info.node_id].gref = info.desc_gref;
      printk(KERN_CRIT "[ioctl] reference recorded %u\n", rdomh[info.node_id].gref);
       
    } else if(info.op == RUNMAP) { //it's a remote unmap
	printk(KERN_CRIT "[ioctl] this is unmap operation\n");
	e = node_lookup(info.node_id);
	if(mr_unmap(e, 0)) {
	    printk(KERN_CRIT "[ioctl] unmapping failed\n");
	    return -1;
	}
    } else {
	printk(KERN_CRIT "[ioctl] this is mmap operation\n");
	current_entry = node_lookup(info.node_id);
	printk(KERN_CRIT "[ioctl] shm_ioctl: domid = %d, nid = %d, gref = %d\n",
	       current_entry->domid, current_entry->nid, current_entry->gref);
	if(current_entry == NULL) {
	    printk(KERN_CRIT "[sonuma_drv] domain not found\n");
	    return -1;
	}
    }

    return 0;
}

static int shm_mmap(struct file *filp, struct vm_area_struct *vma) {
    int ret;
    long length = vma->vm_end - vma->vm_start; 
    
    printk(KERN_CRIT "[shm_mmap] vma size %lu\n", (unsigned long)length);
    
    // map the physically contiguous memory area into the caller's address space
    if(current_entry == NULL) { //map local memory
      rdomh[mynid].domid = -1;
      rdomh[mynid].nid = -1;
      printk(KERN_CRIT "[shm_mmap] mapping local memory region\n");
      if ((ret = remap_pfn_range(vma,
				 vma->vm_start,
				 virt_to_phys((void *)local_mr->region) >> PAGE_SHIFT,
				 length,
				 vma->vm_page_prot)) < 0) {
	return -EINVAL;
      }
    } else {
      printk(KERN_CRIT "[sonuma_drv] shm_mmap: mapping remote memory region\n");
      //memmory map granted page directly to the user
      printk(KERN_CRIT "[sonuma_drv] shm_mmap: domid = %d, nid = %d, gref = %d\n",
	     current_entry->domid, current_entry->nid, current_entry->gref);
      
      ret = mr_map(current_entry->domid, current_entry->gref, 0, current_entry, vma);
      current_entry = NULL;
      if(ret) {
	printk(KERN_CRIT "[shm_mmap] mapping remote region failed\n");
	
	return -EINVAL;
      }
    }

    current_entry = NULL;
    
    printk(KERN_CRIT "shm_mmap: memory is mapped\n");

    return 0;
}

static int shm_open(struct inode *inode, struct file *filp) {    
    return 0;
}

static int shm_release(struct inode *inode, struct file *filp) {
    printk(KERN_CRIT "[sonuma_drv] shm_release\n");
    return 0;
}

static void __exit rmc_exit(void) {
    int i;
    
    TRACE_ENTRY;
    
    write_xenstore(0);
    
    printk("[sonuma_drv] rmc_exit: destroying the fabric\n");
    
    //release resources
    for(i=0;i<num_of_doms;i++) {
	printk(KERN_CRIT "[sonuma_drv] rmc_exit: rdomh[i]->nid = %d\n", rdomh[i].nid);
	if(rdomh[i].nid != mynid) {
	    Entry *e = node_lookup(rdomh[i].nid);
	    printk(KERN_CRIT "[sonuma_drv] rmc_exit: releasing mr references\n");
	    if(e != NULL)
		destroy_mr_reference((mr_reference_info_t *)e->mr_info);
	}
    }
    
    //destroy local region
    //mr_destroy(local_mr);
    
    net_exit();
    
    printk(KERN_CRIT "table cleaned\n");
    
    // remove the character deivce
    cdev_del(&mmap_cdev);
    unregister_chrdev_region(mmap_dev, 1);
    
    printk(KERN_CRIT "Exiting xenloop module.\n");
    TRACE_EXIT;
}

static int __init rmc_init(void) {
    int rc = 0;
    int ret, i;
    
    printk(KERN_CRIT "[sonuma_drv] rmc_init ->\n");
    
    printk(KERN_CRIT "[sonuma_drv] this node's ID is %u\n", mynid);
    printk(KERN_CRIT "[sonuma_drv] page_cnt_log2 is %u\n", page_cnt_log2);

    memset(rdom_connected, 0, MAX_NODE_CNT*sizeof(unsigned));
    
    mydomid = get_my_domid();
    printk(KERN_CRIT "[sonuma_drv] rmc_init: my dom id is %d\n", mydomid);

    if ((rc = net_init()) < 0) {
	printk(KERN_CRIT "[sonuma_drv] session_init(): net_init failed\n");
	goto out;
    }

    for(i=0; i<MAX_NODE_CNT; i++) {
      rdomh[i].initialized = 0;
    }
    
    printk(KERN_CRIT "[sonuma_drv] network initialized\n");
    
    write_xenstore(1);
    
    DPRINTK("[sonuma_drv] Remote memory driver successfully initialized!\n");
    
    //create memory region
    //local_mr = mr_create(page_cnt_log2);
    mr_init();
    
    //BUG_ON(!local_mr);
  
    //register device
    if ((ret = alloc_chrdev_region(&mmap_dev, 0, 1, "mmap")) < 0) {
	printk(KERN_ERR "[sonuma_drv] could not allocate major number for mmap\n");
	goto out;
    }
    
    //initialize the device structure and register the device with the kernel
    cdev_init(&mmap_cdev, &shmmap_fops);
    if ((ret = cdev_add(&mmap_cdev, mmap_dev, 1)) < 0) {
	printk(KERN_ERR "[sonuma_drv] could not allocate chrdev for mmap\n");
    }
    
    return ret;
    
 out:
    TRACE_EXIT;
    return rc;
}

module_init(rmc_init);
module_exit(rmc_exit);

MODULE_LICENSE("GPL");
