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


#include "maptable.h"
#include "debug.h"
//#include "rmccore.h"
#include "main.h"

extern void send_destroy_chn_msg(u8 *dest_mac); 
extern wait_queue_head_t swq;

static DEFINE_SPINLOCK(glock);

ulong hash(u8 *pmac){
  //printk(KERN_CRIT "hash function invoked!\n");
  return (pmac[3] + pmac[4] + pmac[5]) % HASH_SIZE;
}


int equal(void *pmac1, void *pmac2)
{
  if (memcmp(pmac1, pmac2, ETH_ALEN) == 0)
    return 1;
  else
    return 0;
};

inline Entry* get_dom(HashTable *ht) {
  int i;
  struct list_head * x, * y;
  Entry * e;

  for(i = 0; i < HASH_SIZE; i++) { 
    list_for_each_safe(x, y, &(ht->table[i].bucket)) {
      e = list_entry(x, Entry, mapping);
      if(e != NULL) {
	return e;
      }
    }
  }

  return NULL;
}

inline Entry* lookup_dom(HashTable * ht, domid_t rdomid)
{
    /*
    int i;
  struct list_head * x, * y;
  Entry * e;

  for(i = 0; i < HASH_SIZE; i++) { 
    list_for_each_safe(x, y, &(ht->table[i].bucket)) {
      e = list_entry(x, Entry, mapping);
      if(rdomid ==  e->his_bfh->remote_domid) {
	return e;
      }
    }
  }
  */
  return NULL;
}

//epfl
inline Entry *insert_table(HashTable * ht, void * key, u8 domid) 
{
	Bucket * b = &ht->table[hash(key)];
	Entry * e;
	ulong flags;

	e = kmem_cache_alloc((struct kmem_cache *)ht->entries, GFP_ATOMIC);
	//e = (Entry *)__get_free_page(GFP_KERNEL);
	BUG_ON(!e);
	memcpy(e->mac, key, ETH_ALEN);
	e->domid = domid;
	printk(KERN_CRIT "[maptable] insert_table: domid id %d\n", e->domid);
	//e->timestamp = jiffies;
	//e->ack_timer = NULL;
	//e->status = RMC_STATUS_INIT;
	//e->listen_flag = 0xff;
	//e->his_bfh = NULL;
	//e->mine_bfh = NULL;
	//e->retry_count = 0;
	
	spin_lock_irqsave(&glock, flags);
	list_add(&e->mapping, &(b->bucket));
	ht->count++;
	spin_unlock_irqrestore(&glock, flags);
	
	return e;
}



inline void remove_entry(HashTable *ht, Entry *e, struct list_head *x) {
	ulong flags;

	TRACE_ENTRY;
	spin_lock_irqsave(&glock, flags);
	list_del(x);
	ht->count--;
	spin_unlock_irqrestore(&glock, flags);
	/*
	if ((e->his_bfh) || (e->mine_bfh)) {
		if(e->listen_flag) {
		    //rmc_destroy_channel(e->his_bfh);
		    //rmc_destroy_channel(e->mine_bfh);
		} else {
		    //rmc_disconnect(e->his_bfh);
		    //rmc_disconnect(e->mine_bfh);
		}
		e->his_bfh = NULL;
		e->mine_bfh = NULL;
	}
	e->status  = RMC_STATUS_INIT;

	if (e->ack_timer)
		del_timer_sync(e->ack_timer);
	*/
	kmem_cache_free((struct kmem_cache *)ht->entries, e);
	DPRINTK("Delete Guest: deleted one guest mac =" MAC_FMT " Domid = %d.\n", \
		 MAC_NTOA(e->mac), e->domid);
	TRACE_EXIT;
}

inline Entry* lookup_bfh(HashTable * ht, void * key)
{
	int i;
	struct list_head * x, * y;
	Entry * e;

	for(i = 0; i < HASH_SIZE; i++) { 
		list_for_each_safe(x, y, &(ht->table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
			/*
			if(key ==  e->mine_bfh) {
				return e;
			}
			*/
		}
	}
	return NULL;
}



inline void * lookup_table(HashTable * ht, void * key) 
{ 
	Entry * d = NULL;
	//printk(KERN_CRIT "lookup_table 1!\n");
	//printk(KERN_CRIT "lookup_table hash(key) = %d!\n", hash(key));
	Bucket * b = &ht->table[hash(key)];

	//printk(KERN_CRIT "lookup_table 2!\n");

	if(!list_empty(&b->bucket)) {
		struct list_head * x;
		Entry * e;
		//printk(KERN_CRIT "lookup_table 3!\n");
		list_for_each(x, &(b->bucket)) {
		  //printk(KERN_CRIT "lookup_table 4!\n");
			e = list_entry(x, Entry, mapping);
			//printk(KERN_CRIT "lookup_table 5!\n");
			if(equal(key, (u8 *) e->mac)) {
			  //printk(KERN_CRIT "lookup_table 6!\n");
				d = e;
				break;
			}
		}
	}
	return d;
}

/*
inline int has_suspend_entry(HashTable * ht) 
{
	int i;
	Entry *e;
	struct list_head *x, *y;
	Bucket * table = ht->table;
	
	for(i = 0; i < HASH_SIZE; i++) { 
		list_for_each_safe(x, y, &(table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
			if (e->status == RMC_STATUS_SUSPEND) 
				return 1;
		}
	}
	return 0;
}

inline void mark_suspend(HashTable * ht) 
{
	int i;
	Entry *e;
	struct list_head *x, *y;
	Bucket * table = ht->table;
	TRACE_ENTRY;
	for(i = 0; i < HASH_SIZE; i++) {
		list_for_each_safe(x, y, &(table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
			if (check_descriptor(e->bfh)) {
				BF_SUSPEND_IN(e->bfh) = 1;
				BF_SUSPEND_OUT(e->bfh) = 1;
				bf_notify(e->bfh->port);
			}
			e->status = RMC_STATUS_SUSPEND;
		}
	}
	TRACE_EXIT;
}


void notify_all_bfs(HashTable * ht) 
{
	int i;
	Entry *e;
	struct list_head *x, *y;
	Bucket * table = ht->table;

	TRACE_ENTRY;

	for(i = 0; i < HASH_SIZE; i++) {
		list_for_each_safe(x, y, &(table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
			if ( check_descriptor(e->bfh) && (xf_size( e->bfh->out ) > 0) )
					bf_notify(e->bfh->port);
		}
	}

	TRACE_EXIT;
}


inline void check_timeout(HashTable * ht)
{
	int i, found = 0;
	Entry *e;
	struct list_head *x, *y;
	Bucket * table = ht->table;

	for(i = 0; i < HASH_SIZE; i++) { 
		list_for_each_safe(x, y, &(table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
	 		if ((jiffies - e->timestamp) > (5*DISCOVER_TIMEOUT*HZ)) {
				if (check_descriptor(e->bfh)) {
					BF_SUSPEND_IN(e->bfh) = 1;
					BF_SUSPEND_OUT(e->bfh) = 1;
				}
				e->status = RMC_STATUS_SUSPEND;
				found = 1;
			}
		}
	}
	if (found) 
		wake_up_interruptible(&swq);
}


inline void update_table(HashTable * ht, u8 *mac, int mac_count) 
{
	int i,j,found = 0;
	Entry *e;
	void *p;
	struct list_head *x, *y;
	Bucket * table = ht->table;

	
	for(j = 0; j < HASH_SIZE; j++) { 
		list_for_each_safe(x, y, &(table[j].bucket)) {
			e = list_entry(x, Entry, mapping);
			for(i = 0, p = mac;  i < mac_count; i++, p+= ETH_ALEN) {
				if (equal(p, e->mac)) {
					e->timestamp = jiffies;
					found = 1;
					break;
				}
				
			}
			
			if (found) {
				found = 0;
				continue;
			}
			
			if (check_descriptor(e->bfh)) {
				BF_SUSPEND_IN(e->bfh) = 1;
				BF_SUSPEND_OUT(e->bfh) = 1;
			}
			e->status = RMC_STATUS_SUSPEND;
			found = 0;
			wake_up_interruptible(&swq);
		}
	}
}

*/

int init_hash_table(HashTable * ht, char * name) 
{
	int i;
	
	//printk(KERN_CRIT "init_hash_table!\n");

	ht->count 	= 0;
	//epfl
	//ht->entries = kmem_cache_create(name, sizeof(Entry), 0, 0, NULL, NULL);
	ht->entries = (struct kmem_cache_t *)kmem_cache_create(name, sizeof(Entry), 0, 0, NULL);
	//

	if(!ht->entries) {
		EPRINTK("hashtable(): slab caches failed.\n");
		return -ENOMEM;
	}

	for(i = 0; i < HASH_SIZE; i++) {
		INIT_LIST_HEAD(&(ht->table[i].bucket));
	}

	return 0;
}
//EXPORT_SYMBOL(init_hash_table);
/*
void clean_suspended_entries(HashTable * ht)
{
	int i;
	Entry *e;
	struct list_head *x, *y;
	Bucket * table = ht->table;

	for(i = 0; i < HASH_SIZE; i++) { 
		list_for_each_safe(x, y, &(table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
			if (e->status == RMC_STATUS_SUSPEND) {
				remove_entry(ht, e, x);
			} 
		}
	}
}

*/

void clean_table(HashTable * ht) 
{
	int i;
	Entry *e;
	struct list_head *x, *y;
	Bucket * table = ht->table;

	for(i = 0; i < HASH_SIZE; i++) { 
		list_for_each_safe(x, y, &(table[i].bucket)) {
			e = list_entry(x, Entry, mapping);
			remove_entry(ht, e, x);
		}
	}

	//BUG_ON(kmem_cache_destroy(ht->entries));
	kmem_cache_destroy((struct kmem_cache *)ht->entries);
}

