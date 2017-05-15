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


#ifndef _DEBUG_H_
#define _DEBUG_H_


#include <linux/netdevice.h>

//#define DEBUG

#ifdef DEBUG
#define TRACE_ENTRY printk(KERN_CRIT "Entering %s\n", __func__)
#define TRACE_ENTRY_ONCE do{ static int once = 1; if (once){ TRACE_ENTRY; once = 0; } }while(0)
#define TRACE_EXIT  printk(KERN_CRIT "Exiting %s\n", __func__)
#define DUMP_STACK_ONCE do{ static int once = 1; if (once){ dump_stack(); once = 0; } }while(0)
#define DB( x, args... ) printk(KERN_CRIT "DEBUG: %s: line %d: " x, __FUNCTION__ , __LINE__ , ## args ); 
#define DPRINTK( x, args... ) printk(KERN_CRIT "%s: line %d: " x, __FUNCTION__ , __LINE__ , ## args );
#else

#define TRACE_ENTRY do {} while (0)
#define TRACE_ENTRY_ONCE do {} while (0)
#define TRACE_EXIT  do {} while (0)
#define DUMP_STACK_ONCE do{} while(0)
#define DB(x, args...) do{} while(0)
#define DPRINTK( x, args... )
#endif

#define TRACE_ERROR printk(KERN_CRIT "ERROR: Exiting %s\n", __func__)
#define EPRINTK( x, args... ) printk(KERN_CRIT "ERROR %s: line %d: " x, __FUNCTION__ , __LINE__ , ## args );  

/* DB is for all DEBUG info that only needed at debug time
 * EPRINTK is for all ERROR messages
 * DPRINTK is for all necessary init/end status milestone for user
 */



#define MAC_FMT				"%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_NTOA(addr) 			addr[0], \
					addr[1], \
					addr[2], \
					addr[3], \
					addr[4], \
					addr[5]



#endif /* _DEBUG_H_ */

