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
 *  Remote memory mapper for the Xen hypervisor
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/genhd.h>

#include <xen/hypercall.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>

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

#include "mr_alloc.h"

static int bad_address(void *p)
{
  unsigned long dummy;
  return probe_kernel_address((unsigned long*)p, dummy);
}

static unsigned long any_v2p(unsigned long vaddr)
{
  struct mm_struct *mm = current->mm;
  pgd_t *pgd = pgd_offset(mm, vaddr);
  //p4d_t *p4d; // MARK: uncomment for 5-level PTEs
  pud_t *pud;
  pmd_t *pmd;
  pte_t *pte;

  struct page *pg;
  unsigned long paddr = 0;

  if (bad_address(pgd)) {
    printk(KERN_ALERT "[any_v2p] Alert: bad address of pgd %p\n", pgd);
    goto bad;
  }
  if (!pgd_present(*pgd)) {
    printk(KERN_ALERT "[any_v2p] Alert: pgd not present %lu\n", (long unsigned int)pgd);
    goto out;
  }

  // MARK: updates this to work with kernel versions w. PTE-5 level
  /*
  p4d = p4d_offset(pgd, vaddr);
  if (bad_address(p4d)) {
    printk(KERN_ALERT "[any_v2p] Alert: bad address of p4d %p\n", p4d);
    goto bad;
  }
  if (!p4d_present(*p4d) || p4d_large(*p4d)) {
    printk(KERN_ALERT "[any_v2p] Alert: p4d not present %lu\n", (long unsigned int)p4d);
    goto out;
  }
  */
  // END MARk

  pud = pud_offset(pgd, vaddr);
  if (bad_address(pud)) {
    printk(KERN_ALERT "[any_v2p] Alert: bad address of pud %p\n", pud);
    goto bad;
  }
  if (!pud_present(*pud) || pud_large(*pud)) {
    printk(KERN_ALERT "[any_v2p] Alert: pud not present %lu\n", (long unsigned int)pud);
    goto out;
  }

  pmd = pmd_offset(pud, vaddr);
  if (bad_address(pmd)) {
    printk(KERN_ALERT "[any_v2p] Alert: bad address of pmd %p\n", pmd);
    goto bad;
  }
  if (!pmd_present(*pmd) || pmd_large(*pmd)) {
    printk(KERN_ALERT "[any_v2p] Alert: pmd not present %lu\n", (long unsigned int)pmd);
    goto out;
  }

  pte = pte_offset_kernel(pmd, vaddr);
  if (bad_address(pte)) {
    printk(KERN_ALERT "[any_v2p] Alert: bad address of pte %p\n", pte);
    goto bad;
  }
  if (!pte_present(*pte)) {
    printk(KERN_ALERT "[any_v2p] Alert: pte not present %lu\n", (long unsigned int)pte);
    goto out;
  }

  pg = pte_page(*pte);
  paddr = (pte_val(*pte) & PHYSICAL_PAGE_MASK) | (vaddr&(PAGE_SIZE-1));

 out:
  return paddr;
 bad:
  printk(KERN_ALERT "[any_v2p] Alert: Bad address\n");
  return 0;
}

void mr_init()
{
  int max_frames = gnttab_max_grant_frames();
  printk(KERN_CRIT "[mr_init] maximum number of frames is %u\n", max_frames);
}

//here we release all grefs and free the associated memory
int mr_destroy(memory_region_t *mr)
{
    free_page((unsigned long)mr->region);
    kfree(mr);
    
    return 0;
}

memory_region_t *mr_create(unsigned int page_count_log2)
{
    memory_region_t *mr = NULL;
    
    printk(KERN_CRIT "[mr_alloc] mr_create ->\n");

    if(sizeof(memory_region_t) > PAGE_SIZE) 
	BUG(); 
	
    if((1 << page_count_log2) > MAX_REGION_PAGES) {
	goto err;
    }

    mr = kmalloc(sizeof(memory_region_t), GFP_KERNEL);
    if(!mr) {
	printk(KERN_CRIT "[mr_alloc] out of memory\n");
	goto err;
    }
    memset(mr, 0, sizeof(memory_region_t));	

    //allocate 2^page_count_log2 pages
    mr->region = (void *)vmalloc(PAGE_SIZE << page_count_log2);
    //could also try: __get_free_pages(GFP_KERNEL, page_count_log2);
    if(!mr->region) {
	printk(KERN_CRIT "[mr_alloc] cannot allocate buffer memory pages\n");
	goto err;
    }

    mr->num_pages = (1 << page_count_log2);
    printk(KERN_CRIT "[mr_alloc] number of pages allocate %d\n", mr->num_pages);
	
    return mr;

 err:
    printk(KERN_CRIT "[mr_alloc] allocation failed\n");
    return NULL;
}

void destroy_mr_reference(mr_reference_info_t *mr_info)
{
    if(!mr_info) {
	printk(KERN_CRIT "[mr_alloc] destroy_mr_reference: mr reference is NULL\n");
	return;
    }

    printk(KERN_CRIT "[mr_alloc] destroy_mr_reference: pages allocated %d\n",
	   mr_info->root_mr_ref->num_pages);

#ifdef END_FOREIGN_ACCESS
    for(i=0; i<mr_info->root_mr_ref->num_pages; i++) {
      for(j=0; j<mr_info->mr_refs[i]->num_pages; j++) {
	gnttab_end_foreign_access_ref(mr_info->mr_refs[i]->desc_grefs[j], 0);
      }
      gnttab_end_foreign_access_ref(mr_info->root_mr_ref->desc_grefs[i], 0);
    }
    gnttab_end_foreign_access_ref(mr_info->root_mr_ref->root_desc_gref, 0);
#endif
    
    printk(KERN_CRIT "[mr_alloc] destroy_mr_reference: removed all grant references\n");
}

mr_reference_info_t *create_mr_reference(unsigned long mem, domid_t remote_domid)
{  
  int i, j;
  unsigned long paddr, pfn;
   
  mr_reference_info_t *mr_info = kmalloc(sizeof(mr_reference_info_t), GFP_KERNEL);
  
  printk(KERN_CRIT "[create_mr_reference] create_mr_reference <-, domain = %u\n",
	 remote_domid);
   
  if(sizeof(memory_region_reference_t) > PAGE_SIZE)
    BUG();
   
  mr_info->root_mr_ref = (memory_region_reference_t *)__get_free_page(GFP_KERNEL); //one page needed
  if(!mr_info->root_mr_ref) {
    printk(KERN_CRIT "[create_mr_reference] cannot allocate memory for descriptor\n");
    goto err;
  }
  memset(mr_info->root_mr_ref, 0, sizeof(memory_region_reference_t));
   
  printk(KERN_CRIT "[create_mr_reference] allocated a page for the descriptor\n");
   
  //grant root page
  mr_info->root_mr_ref->num_pages = MAX_ROOT_DESC_PAGES;
  mr_info->root_mr_ref->root_desc_gref =
    gnttab_grant_foreign_access(remote_domid,
				virt_to_mfn(mr_info->root_mr_ref),
				0);
  if(mr_info->root_mr_ref->root_desc_gref < 0) {
    printk(KERN_CRIT "cannot share descriptor gref page\n");
    goto err;
  }
  
  //grant second level pages (descriptor pages)
  for(i=0; i<mr_info->root_mr_ref->num_pages; i++) {
    //allocate a descriptor page
    mr_info->mr_refs[i] = (memory_region_reference_t *)__get_free_page(GFP_KERNEL);
    if(!mr_info->mr_refs[i]) {
      printk(KERN_CRIT "[create_mr_reference] cannot allocate memory for descriptor\n");
      goto err;
    }
    memset(mr_info->mr_refs[i], 0, sizeof(memory_region_reference_t));
    mr_info->mr_refs[i]->num_pages = MAX_DESC_PAGES;
     
    mr_info->root_mr_ref->desc_grefs[i] =
      gnttab_grant_foreign_access(remote_domid,
				  virt_to_mfn(mr_info->mr_refs[i]),
				  0);
    if(mr_info->root_mr_ref->desc_grefs[i] < 0) {
      printk(KERN_CRIT "[create_mr_reference] cannot share region page\n");
      while(--i)
	gnttab_end_foreign_access_ref(mr_info->root_mr_ref->desc_grefs[i], 0);
      gnttab_end_foreign_access_ref(mr_info->root_mr_ref->root_desc_gref, 0);
      goto err;
    }
  }
  
  //grant access to application pages
  printk(KERN_CRIT "[create_mr_reference] reference to descriptors granted\n");
  for(i=0; i<MAX_REGION_PAGES; i++) {
    paddr = any_v2p(mem + (i*PAGE_SIZE));
    pfn = paddr >> PAGE_SHIFT;
    
    mr_info->mr_refs[i/MAX_DESC_PAGES]->desc_grefs[i%MAX_DESC_PAGES] =
      gnttab_grant_foreign_access(remote_domid,
				  pfn_to_mfn(pfn),
				  0);
     
    if(mr_info->mr_refs[i/MAX_DESC_PAGES]->desc_grefs[i%MAX_DESC_PAGES] < 0) {
      printk(KERN_CRIT "[create_mr_reference] cannot share region page\n");
      while(--i)
	gnttab_end_foreign_access_ref(mr_info->mr_refs[i/MAX_DESC_PAGES]->desc_grefs[i%MAX_DESC_PAGES], 0);     
      for(j=0; j<mr_info->root_mr_ref->num_pages; j++) {
	gnttab_end_foreign_access_ref(mr_info->root_mr_ref->desc_grefs[j], 0);
      }
      gnttab_end_foreign_access_ref(mr_info->root_mr_ref->root_desc_gref, 0);
      goto err;
    }
  }
  
  printk(KERN_CRIT "[create_mr_reference] ALL data pages GRANTED\n");
  return mr_info;
   
 err:
  
  return NULL;
}

static int map_grant_pages(Entry *e)
{
  int i, err = 0;
  memory_region_reference_t *mr_ref;
    
  printk(KERN_CRIT "[map_grant_pages] map_grant_pages\n");
    
  //allocate pages
  e->pages = kcalloc(e->page_cnt, sizeof(e->pages[0]), GFP_KERNEL);
  //if(alloc_xenballooned_pages(e->page_cnt, e->pages, false /* lowmem */))
  if(alloc_xenballooned_pages(e->page_cnt, e->pages)) // MARK: removed lowmem?
    goto err;
   
  //map
  for (i = 0; i < e->page_cnt; i++) {
    unsigned level;
    unsigned long address = (unsigned long)pfn_to_kaddr(page_to_pfn(e->pages[i]));
    pte_t *ptep;
    u64 pte_maddr = 0;
    BUG_ON(PageHighMem(e->pages[i]));

    ptep = lookup_address(address, &level);
    pte_maddr = arbitrary_virt_to_machine(ptep).maddr;
	
    //figure out which descriptor
    mr_ref = e->rdescriptor_vmarea[i/MAX_DESC_PAGES]->addr;
    
    gnttab_set_map_op(&e->kmap_ops[i], pte_maddr,
		      e->flags |
		      GNTMAP_host_map |
		      GNTMAP_contains_pte,
		      mr_ref->desc_grefs[i%MAX_DESC_PAGES],
		      e->domid);
  }
  
  err = gnttab_map_refs(e->map_ops, e->kmap_ops,
			  e->pages, e->page_cnt);
  if (err) {
    printk(KERN_CRIT "[map_grant_pages] gnttab_map_refs failed\n");
    return err;
  }
    
  for (i = 0; i < e->page_cnt; i++) {
    if (e->map_ops[i].status)
      err = -EINVAL;
    else {
      BUG_ON(e->map_ops[i].handle == -1);
      e->unmap_ops[i].handle = e->map_ops[i].handle;
    }
  }
 err:
  return err;
}

static int find_grant_ptes(pte_t *pte, pgtable_t token,
			   unsigned long addr, void *data)
{
  Entry *e = (Entry *)data;
  unsigned int pgnr = (addr - e->vma->vm_start) >> PAGE_SHIFT;
  int flags = e->flags | GNTMAP_application_map | GNTMAP_contains_pte;
  u64 pte_maddr;
  
  //descriptor reference
  int desc_page_id = pgnr/MAX_DESC_PAGES;
  
  memory_region_reference_t *mr_ref = e->rdescriptor_vmarea[desc_page_id]->addr;
  
  BUG_ON(pgnr >= e->page_cnt);
  pte_maddr = arbitrary_virt_to_machine(pte).maddr;
  
  gnttab_set_map_op(&e->map_ops[pgnr], pte_maddr, flags,
		    mr_ref->desc_grefs[pgnr%MAX_DESC_PAGES],
		    e->domid);
  gnttab_set_unmap_op(&e->unmap_ops[pgnr], pte_maddr, flags,
		      -1 /* handle */);
  return 0;
}

bool mr_map(domid_t rdomid, int rgref, int rport, Entry *e, struct vm_area_struct *vma)
{
  memory_region_reference_t *mr_ref = NULL;
  struct gnttab_map_grant_ref map_op;
  
  int ret, err, i;
  
  TRACE_ENTRY;
  
  printk(KERN_CRIT "[mr_map] -> rgref is %d\n", rgref);
  printk(KERN_CRIT "[mr_map] about to allocate memory for the regions\n");
  e->root_rdescriptor_vmarea = alloc_vm_area(PAGE_SIZE, NULL);

  printk(KERN_CRIT "[mr_map] descriptor memory allocated\n");
  
  if(!e->root_rdescriptor_vmarea) {
    EPRINTK("error: cannot allocate memory for descriptor OR FIFO\n");
    goto err;
  }

  gnttab_set_map_op(&map_op, (unsigned long)e->root_rdescriptor_vmarea->addr, 
		    GNTMAP_host_map, rgref, rdomid);
  ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &map_op, 1);
  if( ret || (map_op.status != GNTST_okay) ) {
    EPRINTK("HYPERVISOR_grant_table_op failed ret = %d status = %d\n", ret, map_op.status);
    goto err;
  }
  
  mr_ref = e->root_rdescriptor_vmarea->addr;
  
  printk(KERN_CRIT "[mr_map] mr_map: memory mapped the root page\n");
    
  //memory map other pages
  for(i=0; i<mr_ref->num_pages; i++) {
    e->rdescriptor_vmarea[i] = alloc_vm_area(PAGE_SIZE, NULL);
    if(!e->rdescriptor_vmarea[i]) {
      EPRINTK("error: cannot allocate memory for descriptor OR FIFO\n");
      goto err;
    }
      
    gnttab_set_map_op(&map_op, (unsigned long)e->rdescriptor_vmarea[i]->addr, 
		      GNTMAP_host_map, mr_ref->desc_grefs[i], rdomid);
    ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &map_op, 1);
    if( ret || (map_op.status != GNTST_okay) ) {
      EPRINTK("HYPERVISOR_grant_table_op failed ret = %d status = %d\n",
	      ret, map_op.status);
      goto err;
    }
  }

  printk(KERN_CRIT "[mr_map] allocated and granted descriptor pages\n");
    
  //descriptor
  e->dhandle = map_op.handle;
  e->page_cnt = MAX_REGION_PAGES;
  
  //user area
  vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
  vma->vm_flags |= VM_DONTCOPY;
    
  e->vma = vma;
  e->flags = GNTMAP_host_map;
    
  printk(KERN_CRIT "[mr_map] number of pages to map is %u\n", mr_ref->num_pages);
  
  //start with mapping the granted pages into the vma area
  err = apply_to_page_range(vma->vm_mm, vma->vm_start,
			    vma->vm_end - vma->vm_start,
			    find_grant_ptes, e);
  if (err) {
    printk(KERN_WARNING "[mr_map] find_grant_ptes() failure.\n");
    goto err;
  }
    
  //map the granted pages
  err = map_grant_pages(e);
  if (err) {
    printk(KERN_CRIT "[mr_map] failed to map pages into user space\n");
    goto err;
  }
  
  printk(KERN_CRIT "[mr_map] mapped pages, success\n");
  printk(KERN_CRIT "[mr_map] <-\n");
  
  TRACE_EXIT;
  return false;
    
 err:
  if(e->root_rdescriptor_vmarea) {
    free_vm_area(e->root_rdescriptor_vmarea);
    for(i=0; i<MAX_DESC_PAGES; i++)
      free_vm_area(e->rdescriptor_vmarea[i]);
  }
  
  TRACE_ERROR;
  return true;
}


bool mr_unmap(Entry *e, unsigned long addr)
{
  int i;
  int err = 0;

  // MARK: Added specific structure to remove compilation warnings.
    struct gnttab_unmap_grant_ref* kunmap_tmp_ops = kcalloc(1,sizeof(struct gnttab_unmap_grant_ref),GFP_KERNEL);
    kunmap_tmp_ops->host_addr = e->kmap_ops->host_addr;
    kunmap_tmp_ops->dev_bus_addr = e->kmap_ops->dev_bus_addr;
    kunmap_tmp_ops->handle = e->kmap_ops->handle;
  
  err = gnttab_unmap_refs(e->unmap_ops,
			  //e->kmap_ops, e->pages,
			  kunmap_tmp_ops, e->pages, // MARK
			  e->page_cnt);
  if (err) {
      kfree(kunmap_tmp_ops);
    return err;
  }
  
  for (i = 0; i < e->page_cnt; i++) {
    if (e->unmap_ops[i].status)
      err = -EINVAL;
    e->unmap_ops[i].handle = -1;
  }
  
  printk(KERN_CRIT "[mr_unmap] mr_unmap: references unmapped\n");
   
  //free ballooned pages
  if (e->pages)
    free_xenballooned_pages(e->page_cnt, e->pages);
  
  printk(KERN_CRIT "[mr_unmap] mr_unmap: free ballooned pages\n");
  kfree(kunmap_tmp_ops);  
  return err;
}
