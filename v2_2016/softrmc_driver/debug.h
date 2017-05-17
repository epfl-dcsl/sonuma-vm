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

#ifndef _DEBUG_H_
#define _DEBUG_H_

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

#endif /* _DEBUG_H_ */

