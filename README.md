# sonuma-vm
Sonuma-vm is an all-software implementation of Scale-Out NUMA, a high-performance rack-scale system for in-memory data processing. Scale-Out NUMA replaces the cache-coherence protocol of ccNUMA with a one-sided (RDMA-like) protocol for explicit access to remote memory (with copy semantics). Such remote memory accesses are layered on top of the NUMA's cache-block request/reply protocol through a specialized on-chip hardware block called remote memory controller (RMC).

Sonuma-vm leverages the NUMA assumption of Scale-Out NUMA to emulate a non-coherent NUMA machine with explicit, one-sided access to remote memory (Scale-Out NUMA) by combining a fully-coherent NUMA machine (ccNUMA) and a virtual machine monitor (hypervisor). Sonuma-vm emulates a multi-node Scale-Out NUMA system using a set of virtual machines (VM) mapped (in terms of VCPU and page frames) to distinct NUMA domains of a giant ccNUMA machine. Sonuma-vm leverages the underlying hypervisor to enable access to remote memory - the memory of other NUMA domains. The RMC is implemented in software using byte copy and runs on a dedicated VCPU in each VM. Sonuma-vm uses hardware virtualization (CPU and IO) to minimize the overhead of emulation.

Sonuma-vm is envisioned as a software development platform for Scale-Out NUMA. It runs applications at wall-clock speed and can approximate the latency of a real RMC device. 

To compile sample code, the following dependency package is necessary: libsonuma (export LIBSONUMA_PATH="path to the sonuma library")

To compile, enter v2_2016 and run (results in ./bin):
make

To clean:
make clean

#server 0 (map remote regions)
insmod ./rmc.ko mynid=0 page_cnt_log2=16
#server 1
insmod ./rmc.ko mynid=1 page_cnt_log2=16

#server 0 (run rmcd)
taskset -c 1 ./rmcd 2 0 &
#server 1
taskset -c 1 ./rmcd 2 1 &

#server 1 (write values to the context)
taskset -c 0 ./bench_server 16777216
#server 0 (read values from the memory of server 1
taskset -c 0 ./bench_sync 1 16777216 4096 r
#server 0 (zero out the memory of server 1, read again to check)
taskset -c 0 ./bench_sync 1 16777216 4096 w
#sever 0 (read from remote memory asynchronously)
taskset -c 0 ./bench_async 1 16777216 4096 r
