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
 *
 *  Authors: 
 *  	Stanko Novakovic <stanko.novakovic@epfl.ch>
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

#include <linux/mm.h>
#include <linux/time.h>
#include <linux/genhd.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include <time.h>

#include "main.h"
#include "debug.h"
#include "mr_alloc.h"

//memory region device
static dev_t mmap_dev;
static struct cdev mmap_cdev;

//my NID
static int mynid = 0;
module_param(mynid, int, 0);

unsigned long apps_mem;

//number of pages to allocate/map
//static int page_cnt_log2 = 4;
//module_param(page_cnt_log2, int, 0);

static int shm_open(struct inode *inode, struct file *filp);
static int shm_release(struct inode *inode, struct file *filp);
static int shm_mmap(struct file *filp, struct vm_area_struct *vma);
static long shm_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

//shmmap methods
static struct file_operations shmmap_fops = {
    .open = shm_open,
    .release = shm_release,
    .mmap = shm_mmap,
    .unlocked_ioctl = shm_ioctl,
    .owner = THIS_MODULE,
};

//domain ids of remote domains -- support up to 16 domains
Entry rdomh[MAX_NODE_CNT];
int num_of_doms = 1;

//current entry being used for mmap operation
Entry *current_entry;

//local memory region
memory_region_t *local_mr;

#ifdef GET_DOMID
static domid_t get_my_domid(void)
{
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
#endif

static long shm_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    Entry *e;    
    ioctl_info_t info;
    mr_reference_info_t *mr_info;
    
    if (copy_from_user(&info, (unsigned char *)arg, sizeof(ioctl_info_t)))
	return -EFAULT;

    printk(KERN_CRIT "[ioctl] info.op = %d, info.node_id = %d\n", info.op, info.node_id);

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
    } else if(info.op == RUNMAP) { 
	printk(KERN_CRIT "[ioctl] this is unmap operation\n");
	e = &rdomh[info.node_id];
	if(mr_unmap(e, 0)) {
	    printk(KERN_CRIT "[ioctl] unmapping failed\n");
	    return -1;
	}
    } else {
	printk(KERN_CRIT "[ioctl] this is mmap operation\n");
	current_entry = &rdomh[info.node_id];
	printk(KERN_CRIT "[ioctl] shm_ioctl: domid = %d, nid = %d, gref = %d\n",
	       current_entry->domid, current_entry->nid, current_entry->gref);
	if(current_entry == NULL) {
	    printk(KERN_CRIT "[sonuma_drv] domain not found\n");
	    return -1;
	}
    }

    return 0;
}

static int shm_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret;
    long length = vma->vm_end - vma->vm_start; 
    
    printk(KERN_CRIT "[shm_mmap] vma size %lu\n", (unsigned long)length);
    
    //map the physically contiguous memory area into the caller's address space
    if(current_entry != NULL) {
      printk(KERN_CRIT "[sonuma_drv] shm_mmap: mapping remote memory region\n");
      //memory map granted page directly to the user
      printk(KERN_CRIT "[sonuma_drv] shm_mmap: domid = %d, nid = %d, gref = %d\n",
	     current_entry->domid, current_entry->nid, current_entry->gref);
      
      ret = mr_map(current_entry->domid, current_entry->gref, 0, current_entry, vma);
      if(ret) {
	printk(KERN_CRIT "[shm_mmap] mapping remote region failed\n");
	return -EINVAL;
      }
    } else {
      printk(KERN_CRIT "[shm_mmap] there's nothing to mmap\n");
      return -EINVAL;
    } 

    current_entry = NULL;
    printk(KERN_CRIT "shm_mmap: memory is mapped\n");

    return 0;
}

static int shm_open(struct inode *inode, struct file *filp)
{
  printk(KERN_CRIT "[sonuma_drv] shm_open\n");
    return 0;
}

static int shm_release(struct inode *inode, struct file *filp)
{
    printk(KERN_CRIT "[sonuma_drv] shm_release\n");
    return 0;
}

static void __exit rmc_exit(void)
{
    int i;
    
    TRACE_ENTRY;
    
    //release resources
    for(i=0;i<num_of_doms;i++) {
	printk(KERN_CRIT "[sonuma_drv] rmc_exit: rdomh[i]->nid = %d\n", rdomh[i].nid);
	if(i != mynid) {
	  Entry *e = &rdomh[i];
	  printk(KERN_CRIT "[sonuma_drv] rmc_exit: releasing mr references\n");
	  if(e != NULL)
	    destroy_mr_reference((mr_reference_info_t *)e->mr_info);
	}
    }
    
    // remove the character deivce
    cdev_del(&mmap_cdev);
    unregister_chrdev_region(mmap_dev, 1);
    
    TRACE_EXIT;
}

static int __init rmc_init(void)
{
    int rc = 0;
    int ret;
    
    //printk(KERN_CRIT "[sonuma_drv] this node's ID is %u\n", mynid);
    //printk(KERN_CRIT "[sonuma_drv] page_cnt_log2 is %u\n", page_cnt_log2);

    mr_init();
    
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
