# sonuma-vm
Sonuma-vm is an all-software implementation of Scale-Out NUMA, a high-performance rack-scale system for in-memory data processing. Scale-Out NUMA replaces the cache-coherence protocol of ccNUMA with a one-sided (RDMA-like) protocol for explicit access to remote memory (with copy semantics). Such remote memory accesses are layered on top of the NUMA's cache-block request/reply protocol through a specialized on-chip hardware block called remote memory controller (RMC).

Sonuma-vm leverages the NUMA assumption of Scale-Out NUMA to emulate a non-coherent NUMA machine with explicit, one-sided access to remote memory (Scale-Out NUMA) by combining a fully-coherent NUMA machine (ccNUMA) and a virtual machine monitor (hypervisor). Sonuma-vm emulates a multi-node Scale-Out NUMA system using a set of virtual machines (VM) mapped (in terms of VCPU and page frames) to distinct NUMA domains of a giant ccNUMA machine. Sonuma-vm leverages the underlying hypervisor to enable access to remote memory - the memory of other NUMA domains. The RMC is implemented in software using byte copy and runs on a dedicated VCPU in each VM. Sonuma-vm uses hardware virtualization (CPU and IO) to minimize the overhead of emulation.

Sonuma-vm is envisioned as a software development platform for Scale-Out NUMA. It runs applications at wall-clock speed and can approximate the latency of a real RMC device. 


### compile<br/>
export LIBSONUMA_PATH="path to libsonuma"<br\>
make (output in ./bin)<br/> 
<br />
### clean<br />
make clean<br/>
<br />
### insert kernel driver to map remote regions<br />
#server 0 (map remote regions)<br />
insmod ./rmc.ko mynid=0 page_cnt_log2=16<br />
#server 1<br />
insmod ./rmc.ko mynid=1 page_cnt_log2=16<br />
<br />

### run RMC daemon
#server 0 (run rmcd)<br />
taskset -c 1 ./rmcd 2 0 &<br />
#server 1<br />
taskset -c 1 ./rmcd 2 1 &<br />
<br />

### read/write from remote memory using sync/async operations
#server 1 (write values to the context)<br />
taskset -c 0 ./bench_server 16777216<br />
#server 0 (read values from the memory of server 1<br />
taskset -c 0 ./bench_sync 1 16777216 4096 r<br />
#server 0 (zero out the memory of server 1, read again to check)<br />
taskset -c 0 ./bench_sync 1 16777216 4096 w<br />
#sever 0 (read from remote memory asynchronously)<br />
taskset -c 0 ./bench_async 1 16777216 4096 r<br />
#sever 0 (write remote memory asynchronously)<br />
taskset -c 0 ./bench_async 1 16777216 4096 w<br />
<br />