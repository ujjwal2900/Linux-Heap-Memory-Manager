Heap Memory Management in Linux

Heap Memory of a process is the continuous part of the virtual address space of the process from which it claims and reclaims memory during runtime.
Glibc APIs to harness this function:
  •	Malloc, calloc, free and realloc
  •	System calls: brk, sbrk
Unlike stack memory which is reclaimed back upon the return of the function automatically, it is programmer’s responsibility to free the dynamic memory after usage.
In C / Cpp allocation and deallocation of memory is performed with the help of malloc, calloc, free and realloc calls.
This project implements a Linux Heap Memory Manager which caters to the memory requirements of the application. It takes care of allocation and deallocation of the memory requested. It can be used to minimize memory fragmentation issues, can be used to detect memory Leaks, keep track of the memory being used by the different structures in our application and optimize our application with memory requirement issues.

Overview of Memory Management in Linux

![image](https://github.com/user-attachments/assets/8fef1e78-f092-4d15-acd5-e2d25828de59)

High Level Diagram of our Linux Memory Manager 

![image](https://github.com/user-attachments/assets/4e4b5e13-d295-4bee-8fe1-20c6236ae3a1)

The Linux Memory Manager Performs the following operations to satisfy the memory requirements of the application: 
  •	Request for VM Page from Kernel.
  •	Maintains a doubly linked lists of Meta Blocks which guards the adjacent data blocks and performs allocation and deallocation of Data Blocks.
  •	Maintains a Priority Queue of Free Data Blocks to satisfy the memory requirements (Follows the Worst Fit Policy of memory allocation)
  •	If a VM page is allocated completely and no space is left to satisfy the memory requested, then request for new VM Page
  •	If a VM Page is completely free and has no region occupied, then return it back to the kernel.
  •	Maintains collection of VM Pages
  •	Collect statistics of Memory usage

Output of Memory statistics using LMM :

![Allocation_and_deallocation](https://github.com/user-attachments/assets/f011b923-d935-4211-8faa-ff0406664fc2)

![large_no_of_allocations](https://github.com/user-attachments/assets/e9e2222e-7dd5-4023-a9be-308c4c35b5eb)





