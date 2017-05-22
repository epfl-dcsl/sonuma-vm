#
# Scale-Out NUMA Open Source License
#
# Copyright (c) 2017, Parallel Systems Architecture Lab, EPFL
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of the Parallel Systems Architecture Lab, EPFL,
#   nor the names of its contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LAB,
# EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

MODULENAME      := rmc

KDIR_sun	:= /lib/modules/$(shell uname -r)/build

ccflags-y      := -I/lib/modules/$(shell uname -r)/build/arch/x86/include/asm -I/lib/modules/$(shell uname -r)/build/include -I/lib/modules/$(shell uname -r)/build/arch/x86/include/asm -I/lib/modules/$(shell uname -r)/build/include/uapi/linux

PWD := $(shell pwd)

ifeq ($(KERNELRELEASE),)
all:
	$(MAKE) MORE_FLAGS='-finline-functions gnu_inline' CFLAGS="-finline-functions" $(ccflags-y) -C $(KDIR_sun) M=$(PWD) modules
	cp ./rmc.ko ../bin/
else
	rmc-objs := main.o mr_alloc.o
	obj-m := rmc.o  
endif

clean:
	rm ./*.o ./*.ko ./*.symvers ./*.order
