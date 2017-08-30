New feature: virtual time support for network emulation

Introduction
============
This patch is the result of our paper ["A Virtual Time System for Linux-container-based Emulation of Software-defined Networks"](www.dummy.com), to be appears on ACM SIGSIM Conference on Principles of Advanced Discrete Simulation (PADS) 2015.

Network emulation is attractive because of the combination of many desired features of network simulators (scalability, low cost, reproducibility, and flexibility) and physical testbeds (high fidelity) when it adopts OS-level virtualization techniques like containers and namespace. Mininet is an examplary emulator widely used the Soft Defined Network community. However, an ordinary network emulator uses the system clock across all the containers, even if a container is not being scheduled to run. This leads to the issue of temporal fidelity, especially with high workloads. 

Virtual time sheds the light on the issue of preserving temporal fidelity for large-scale emulation. The key insight is to trade time with system resources via precisely scaling the time of interactions between containers and physical devices by a factor of N (also called time dilation factor), hence, making an emulated network appear to be N times faster from the viewpoints of applications in the container than it actually is. 

How I Implement Virtual Time
============================
First we need to let every process have its own clock, which is done by adding these variables in task_struct:
```
s64 virtual_start_nsec;
s64 physical_past_nsec;
s64 virtual_past_nsec;
int dilation; /* geq 1 */
```
Dilation describe how slow the virtual clock is. For example, dilation=10 means that when the wall clock passes 10 seconds, the dilated process will think that only 1 second elapsed. The timekeeping part, e.g. maintaining virtual time, is done in
```
static void dilate_nstimeofday(struct timespec *ts);
```
which I add in kernel/time/timekeeping.c.
To expose interface to userspace, I add two system calls 
```
SYSCALL_DEFINE2(set_dilation, unsigned long, dilation, pid_t, ppid);
SYSCALL_DEFINE2(virtualtime_unshare, unsigned long, unshare_flags, unsigned long, dilation);
```
set_dilation changes the value of a process's (or its parent's) dilation. virtualtime_unshare is almost a clone of unshare, except that it now take in dilation value as input. More details can be found in the paper mentioned above.


Acknowledge
===========
My implementation of virtual time is actually inspired by that of Jeremy Lamps, who is a PhD student at UIUC. Great thanks for sharing his source code and providing a very detailed documentation.


Application and Test
====================
I test this patch on my Dell XPS 8700 Desktop running on Ubuntu 14.04 LTS. We integrated virtual time with Mininet and conducted many experimental evaluations. Details can be found in our paper. The integrated Mininet code will soon be published at my [Github](https://github.com/littlepretty) website. To run Mininet with virtual time, a little more patch on tc(traffic control) is needed.

Known Bugs
==========
For now just __getnstimeofday invoke the dilation code, which means that only gettimeofday can return virtual time. The reason is that during our development, this works just fine.
If the application inside container is at a very deep position of the process tree, set_dilation will need to do cascading operations so that the root process's dilation is properly changed. This is not implemented yet.