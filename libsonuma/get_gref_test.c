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

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_REGION_PAGES;
#define RMAP 1
#define GETREF 2
#define RUNMAP 0

typedef struct ioctl_info {
  int op;
  int node_id;
  int desc_gref;
} ioctl_info_t;


int main()
{
  int i;  
  int fd;
  
  if ((fd=open("/root/node", O_RDWR|O_SYNC)) < 0) {
    return -1;
  }
  
  ioctl_info_t info;

  info.op = GETREF;
  info.node_id = 1;
  if(ioctl(fd, 0, (void *)&info) == -1) {
    printf("[soft_rmc] failed to unmap a remote region\n");
    return -1;
  }

  printf("number of grant references %u\n", info.desc_gref);
}
