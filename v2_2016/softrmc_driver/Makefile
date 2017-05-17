MODULENAME      := rmc

KDIR_sun	:= /lib/modules/$(shell uname -r)/build

ccflags-y      := -I/lib/modules/$(shell uname -r)/build/arch/x86/include/asm
ccflags-y_sun	:= -I/lib/modules/$(shell uname -r)/build/include -I/lib/modules/$(shell uname -r)/build/arch/x86/include/asm
ccflags-y_moon	:= -I/lib/modules/$(shell uname -r)/build/include/uapi/linux

PWD := $(shell pwd)

ifeq ($(KERNELRELEASE),)
all:
	$(MAKE) MORE_FLAGS='-finline-functions gnu_inline' CFLAGS="-finline-functions" $(ccflags-y) $(ccflags-y_sun) $(ccflags-y_moon) -C $(KDIR_sun) M=$(PWD) modules
	cp ./rmc.ko ../bin/
else
	rmc-objs := main.o mr_alloc.o
	obj-m := rmc.o  
endif

clean:
	rm ./*.o ./*.ko ./*.symvers ./*.order
