# sonuma-vm
Sonuma-vm is an all-software implementation of Scale-Out NUMA, a high-performance rack-scale system for in-memory data processing. Scale-Out NUMA replaces the cache-coherence protocol of ccNUMA with a one-sided (RDMA-like) protocol for explicit access to remote memory (with copy semantics). Such remote memory accesses are layered on top of the NUMA's cache-block request/reply protocol through a specialized on-chip hardware block called remote memory controller (RMC).

Sonuma-vm leverages the NUMA assumption of Scale-Out NUMA to emulate a non-coherent NUMA machine with explicit, one-sided access to remote memory (Scale-Out NUMA) by combining a fully-coherent NUMA machine (ccNUMA) and a virtual machine monitor (hypervisor). Sonuma-vm emulates a multi-node Scale-Out NUMA system using a set of virtual machines (VM) mapped (in terms of VCPU and page frames) to distinct NUMA domains of a giant ccNUMA machine. Sonuma-vm leverages the underlying hypervisor to enable access to remote memory - the memory of other NUMA domains. The RMC is implemented in software using byte copy and runs on a dedicated VCPU in each VM. Sonuma-vm uses hardware virtualization (CPU and IO) to minimize the overhead of emulation.

Sonuma-vm is envisioned as a software development platform for Scale-Out NUMA. It runs applications at wall-clock speed and can approximate the latency of a real RMC device. 

To compile sample code, the following dependency package is necessary: libsonuma

Need to export LIBSONUMA_PATH=<path to the sonuma library>

To compile, enter v2_2016 and run (results in ./bin):
make

To clean:
make clean
