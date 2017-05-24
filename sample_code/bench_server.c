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
 *  Remote read test for libsonuma and SoftRMC
 */

#include <vector>
#include <algorithm>

#include "sonuma.h"

int main(int argc, char **argv)
{
  uint64_t ctx_size = PAGE_SIZE * PAGE_SIZE; 
  
  uint8_t *ctx = NULL;
  
  int fd = kal_open((char*)RMC_DEV);  
  if(fd < 0) {
    printf("cannot open RMC dev. driver\n");
    return -1;
  }
  
  //context
  kal_reg_ctx(0, &ctx, ctx_size/PAGE_SIZE);
  fprintf(stdout, "Ctx buffer was registered, ctx_size=%d, %d pages.\n",
	  ctx_size, ctx_size*sizeof(uint8_t) / PAGE_SIZE);

  //write the sequence number at the beginning of each page
  for(size_t i = 0; i < ctx_size/PAGE_SIZE; i++) {
      ((uint32_t*)ctx)[(i*PAGE_SIZE)/4] = i;
      printf("wrote this number: %u\n", ((uint32_t*)ctx)[(i*PAGE_SIZE)/4]);
  }
  
  return 0;
}
