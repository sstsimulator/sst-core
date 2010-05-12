#ifndef PPCPIMCALLS_H
#define PPCPIMCALLS_H

#include "pimSysCallDefs.h"
#include "pimSysCallTypes.h"

#define _INLINE_ static inline

/* 
   GCC assembly style:
   asm volatile("" : <output reg> : <input reg> : <clobbered reg> )
*/



//:Fork a new thread (DEPRECATED)
//
// Wrapper around the pim system call. The new function DOES NOT recieve its
// own private stack; that must be set-up ahead of time. If you want a stack,
// try PIM_loadAndSpawnToLocaleStack().
//
//!in: start_routine: Address of the function where the new function will begin execution.
//!in: arg: a void* argument which may be passed to the new function
_INLINE_ int PIM_threadCreate(void* start_routine, void* arg)
{
  int result;
  asm volatile ("mr r3, %1\n" /* Where execution begins */
		"mr r4, %2\n" /* What arguments */
		"li r0, "SS_PIM_FORK_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3, 0\n" /* Collect results*/
                : "=r" (result)
	        : "r" (start_routine), "r" (arg)
		: "r0", "r3", "r4");
  return result;
}

//:Fork a new thread
//
// Wrapper around the pim system call. The new function DOES NOT recieve its
// own private stack; that must be set-up ahead of time. If you want a stack,
// try PIM_loadAndSpawnToLocaleStack().
//
//!in: start_routine: Address of the function where the new function will begin execution.
//!in: arg: a void* argument which may be passed to the new function
//!in: stack: a stack pointer for the new thread (remember, PPC stacks grow DOWN)
_INLINE_ int PIM_threadCreateWithStack(void* start_routine, void* arg, void* stack)
{
    int result;
    asm volatile (
	    "mr r5, r1\n\t" /* save the stack */
	    "mr r1, %3\n\t" /* replace the stack with the new one */
	    "mr r3, %1\n\t" /* Where execution begins */
	    "mr r4, %2\n\t" /* What arguments */
	    "li r0, "SS_PIM_FORK_STR"\n\t" /* Set to syscall */
	    "sc\n\t" /* make the "system call" */
	    "mr %0, r3, 0\n\t" /* Collect results*/
	    "mr r1, r5\n" /* resurrect the stack */
	    : "=r" (result)
	    : "r" (start_routine), "r" (arg), "r" (stack)
	    : "r0", "r3", "r4", "r5");
    return result;
}

//: Spawn to a coprocessor 
//
// Wrapper around the pim system call. spawns a thread to begin on a
// coprocessor, such as a NIC or PIM. Returns a thread ID. 
//
//!in: coProc: coprocessor number. 0 for main processor, 1 for NIC, see pimSysCallTypes.h for full list/enum
//!in: start_routine: Address of the function where the spawned thread will begin
//!in: arg: a void& argument when may be passed to the new function
_INLINE_ int PIM_spawnToCoProc(PIM_coProc coProc, void* start_routine,
			       void* arg)
{
  int result;
  asm volatile ("mr r3, %1\n" /* coprocessor to handle thread */
		"mr r4, %2\n" /* Where execution begins */
		"mr r5, %3\n" /* What arguments */
		"li r0, "SS_PIM_SPAWN_TO_COPROC_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)
	        : "r" (coProc), "r" (start_routine), "r" (arg)
		: "r0", "r3", "r4", "r5");
  return result;
}

//: Spawn to a coprocessor
//
// Wrapper around the pim system call. Spawns a thread to begin on a
// coprocessor, such as a NIC or PIM. Returns a thread ID.
//
//!in: coProc: coprocessor number. 0 for main processor, 1 for NIC, see pimSysCallTypes.h for full list/enum
//!in: start_routine: Address of the function where the spawned thread will begin
//!in: arg: a void* argument when may be passed to the new function
//!in: stack: a stack pointer for the new thread (remember, PPC stacks grow DOWN)
_INLINE_ int PIM_spawnToCoProcWithStack(PIM_coProc coProc,
					void* start_routine,
					void* arg, void* stack)
{
    int result;
    asm volatile (
	    "mr r6, r1\n\t" /* save the stack */
	    "mr r1, %4\n\t" /* replae the stack with the new one */
	    "mr r3, %1\n\t" /* coprocessor to handle thread */
	    "mr r4, %2\n\t" /* Where execution begins */
	    "mr r5, %3\n\t" /* What arguments */
	    "li r0, "SS_PIM_SPAWN_TO_COPROC_STR"\n\t" /* Set to syscall */
	    "sc\n\t" /* make the "system call" */
	    "mr %0, r3\n\t" /* Collect results*/
	    "mr r1, r6\n" /* resurrect the old stack */
	    : "=r" (result)
	    : "r" (coProc), "r" (start_routine), "r" (arg), "r" (stack)
	    : "r0", "r3", "r4", "r5", "r6");
    return result;
}

//: Load registers and spawn to coproc
//
// Same as PIM_spawnToCoProcWithStack(), but allows more registers to be written
_INLINE_ int PIM_loadAndSpawnToCoProcWithStack(PIM_coProc coProc,
				    void* start_routine,
				    void* r3Arg, void* r6Arg, void* r7Arg,
				    void* r8Arg, void* r9Arg, void* stack)
{
    int result;
    asm volatile (
	    "mr r10, r1\n\t" /* save the stack */
	    "mr r1, %8\n\t" /* replace the stack with the new one */
	    "mr r3, %1\n\t" /* coprocessor to handle thread */
	    "mr r4, %2\n\t" /* Where execution begins */
	    "mr r5, %3\n\t" /* argument (ends up in r3) */
	    "mr r6, %4\n\t" /* argument (ends up in r4) */
	    "mr r7, %5\n\t" /* argument (ends up in r5) */
	    "mr r8, %6\n\t" /* argument (ends up in r6) */
	    "mr r9, %7\n\t" /* argument (ends up in r7) */
	    "li r0, "SS_PIM_SPAWN_TO_COPROC_STR"\n\t" /* Set to syscall */
	    "sc\n\t" /* make the "system call" */
	    "mr %0, r3\n\t" /* Collect results*/
	    "mr r1, r10\n" /* resurrect the old stack */
	    : "=r" (result)
	    : "r" (coProc), "r" (start_routine), "r" (r3Arg), "r" (r6Arg),
	    "r" (r7Arg), "r" (r8Arg), "r" (r9Arg), "r" (stack)
	    : "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10");
    return result;
}

//: Load registers and spawn to coproc (DEPRECATED)
//
// Same as PIM_spawnToCoProc(), but allows more registers to be written
_INLINE_ int PIM_loadAndSpawnToCoProc(PIM_coProc coProc, void* start_routine,
				    void* r3Arg, void* r6Arg, void* r7Arg,
				    void* r8Arg, void* r9Arg) {
  int result;
  asm volatile ("mr r3, %1\n" /* coprocessor to handle thread */
		"mr r4, %2\n" /* Where execution begins */
		"mr r5, %3\n" /* argument (ends up in r3) */
		"mr r6, %4\n" /* argument (ends up in r4) */
		"mr r7, %5\n" /* argument (ends up in r5) */
		"mr r8, %6\n" /* argument (ends up in r6) */
		"mr r9, %7\n" /* argument (ends up in r7) */
		"li r0, "SS_PIM_SPAWN_TO_COPROC_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)
	        : "r" (coProc), "r" (start_routine), "r" (r3Arg), "r" (r6Arg),
		  "r" (r7Arg), "r" (r8Arg), "r" (r9Arg)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9");
  return result;
}

//: Loads registers and spawns a thread to a given locale, with a stack
//
// Similar to PIM_loadAndSpawnToCoProc, but gives the thread a stack and
// spawns to a memory locale rather than a device-type.
// On success, it returns the threadID of the newly created thread.
//
// start_routine should be a function with a prototype similar to one of
// the following:
//         void func ();
//         void func1 (arg);
//         void func2 (arg1, arg2);
//         void func3 (arg1, arg2, arg3);
//         void func4 (arg1, arg2, arg3, arg4);
//         void func5 (arg1, arg2, arg3, arg4, arg5);
// The arguments are all passed in via the integer registers (r3, r4, etc.)
// and so cannot be floating point values.
_INLINE_ int PIM_loadAndSpawnToLocaleStack(int locale,
					   void* start_routine, 
					   void* r3Arg, void* r6Arg, 
					   void* r7Arg, void* r8Arg, 
					   void* r9Arg) {
  int result;
  asm volatile ("mr r3, %1\n" /* locale to handle thread */
		"mr r4, %2\n" /* Where execution begins */
		"mr r5, %3\n" /* argument (ends up in r3) */
		"mr r6, %4\n" /* argument (ends up in r4) */
		"mr r7, %5\n" /* argument (ends up in r5) */
		"mr r8, %6\n" /* argument (ends up in r6) */
		"mr r9, %7\n" /* argument (ends up in r7) */
		"li r0, "SS_PIM_SPAWN_TO_LOCALE_STACK_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)			
  	        : "r" (locale), "r" (start_routine), "r" (r3Arg), "r" (r6Arg), 
		  "r" (r7Arg), "r" (r8Arg), "r" (r9Arg)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9");
  return result;
}

//: Loads registers and creates a non-runnable thread on a given locale, with a stack
//
// Similar to PIM_loadAndSpawnToLocaleStack, except the thread is not runnable
// and must be started by using PIM_startStoppedThread
_INLINE_ int PIM_loadAndSpawnToLocaleStackStopped(int locale, void* start_routine,
					void* r3Arg, void* r6Arg, void* r7Arg,
					void* r8Arg, void* r9Arg)
{
    int result;
    asm volatile (
	    "mr r3, %1\n" /* coprocessor to handle thread */
	    "mr r4, %2\n" /* Where execution begins */
	    "mr r5, %3\n" /* argument (ends up in r3) */
	    "mr r6, %4\n" /* argument (ends up in r4) */
	    "mr r7, %5\n" /* argument (ends up in r5) */
	    "mr r8, %6\n" /* argument (ends up in r6) */
	    "mr r9, %7\n" /* argument (ends up in r7) */
	    "li r0, "SS_PIM_SPAWN_TO_LOCALE_STACK_STOPPED_STR"\n" /* Set to syscall */
	    "sc\n" /* make the "system call" */
	    "mr %0, r3\n" /* Collect results */
	    : "=r" (result)
	    : "r" (locale), "r" (start_routine), "r" (r3Arg), "r" (r6Arg),
	      "r" (r7Arg), "r" (r8Arg), "r" (r9Arg)
	    : "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9");
    return result;
}

//: Makes a given thread runnable
_INLINE_ int PIM_startStoppedThread(int tid, int shep)
{
    int result;
    asm volatile (
	    "mr r3, %1\n" /* which thread to start */
	    "mr r4, %2\n" /* which shepherd to use */
	    "li r0, "SS_PIM_START_STOPPED_THREAD_STR"\n" /* set to syscall */
	    "sc\n" /* make the "system call" */
	    "mr %0, r3\n" /* collect results */
	    : "=r" (result)
	    : "r" (tid), "r" (shep)
	    : "r0", "r3", "r4");
    return result;
}

_INLINE_ int PIM_switchAddrMode(PIM_addrMode mode)
{
 int result;
  asm volatile ("mr r3, %1\n" /* mode */
		"li r0, "SS_PIM_SWITCH_ADDR_MODE_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)			
  	        : "r" (mode)
		: "r0", "r3");
  return result;
}

//Fast buffer read
//: Quickly get a buffer full of data from a file
//: return number of bytes read
_INLINE_ unsigned int PIM_fastFileRead (char * filename_addr, 
				    void * buf_addr,
				    unsigned int maxBytes,
				    unsigned int offset) 
{
  unsigned int bytes = 0;
  asm volatile ("mr r3, %1\n" /* store pointer to file name */
		"mr r4, %2\n" /* store pointer to buffer */
		"mr r5, %3\n" /* store max bytes to read */
		"mr r6, %4\n" /* store offset in file to read from */
		"li r0, "SS_PIM_FFILE_RD_STR"\n" /* Set to syscall */
		"sc\n" /* make the call */
		"mr %0, r3\n" /* Collect length of data read */
		: "=r" (bytes)
		: "r" (filename_addr), "r" (buf_addr), "r" (maxBytes), "r" (offset)
		: "r0", "r3", "r4", "r5", "r6", "memory" );
  return bytes;
}

//:Malloc memory
//
// Wrapper around the malloc system call which returns an address
// where the memory was allocated

#define ALLOC_GLOBAL     0
#define ALLOC_LOCAL_ADDR 1
#define ALLOC_LOCAL_ID   2

_INLINE_ void* PIM_alloc(const unsigned int size, const unsigned int type, const unsigned int opt)
{
    register unsigned int result;
    asm volatile ("mr r3, %1\n" /* size */
		  "mr r4, %2\n" /* allocate type */
		  "mr r5, %3\n" /* send option */
		  "li r0, "SS_PIM_MALLOC_STR"\n" /* Set to syscall */
		  "sc\n" /* make the syscall */
		  "mr %0, r3\n" /* collect the results */
		  : "=r" (result)
		  : "r" (size), "r" (type), "r" (opt)
		  : "r0", "r3", "r4", "r5", "memory");
    return (void*)result;
}

_INLINE_ void* PIM_globalMalloc(const unsigned int size)
{
    return PIM_alloc(size, ALLOC_GLOBAL, 0);
}

_INLINE_ void* PIM_localMallocNearAddr(const unsigned int size, const void * addr)
{
    return PIM_alloc(size, ALLOC_LOCAL_ADDR, (unsigned int)addr);
}

_INLINE_ void* PIM_localMallocAtID(const unsigned int size, const unsigned int ID)
{
    return PIM_alloc(size, ALLOC_LOCAL_ID, ID);
}

_INLINE_ void* PIM_fastMalloc(unsigned int size) 
{
  unsigned int result;
  asm volatile ("lwz r3, %1\n" /* size */
		"li r4, 0\n" /*allocate in heap area*/
		"li r0, "SS_PIM_MALLOC_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)			
  	        : "m" (size)
		: "r0", "r3", "r4", "memory");
  return (void*)result;
}

//:Malloc stack memory
//
// Wrapper around the malloc system call which returns an address
// where the memory was allocated - should only be used for thread
// stack creation

_INLINE_ void* PIM_fastStackMalloc(unsigned int size) 
{
  unsigned int result;
  asm volatile ("lwz r3, %1\n" /* size */
		"li r4, 1\n" /*allocate in stack area*/
		"li r0, "SS_PIM_MALLOC_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)			
  	        : "m" (size)
		: "r0", "r3", "r4", "memory");
  return (void*)result;
}

//:Malloc memory
//
// Wrapper around the malloc system call which returns an address
// where the memory was allocated

_INLINE_ unsigned int PIM_fastFreeSize(void *ptr, unsigned int size) 
{
  unsigned int result;
  unsigned int addr = (unsigned int)ptr;
  asm volatile ("lwz r3, %1\n" /* ptr */
		"lwz r4, %2\n" /* size */
		"li r0, "SS_PIM_FREE_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
                : "=r" (result)		
  	        : "m" (addr), "m" (size)
		: "r0", "r3", "r4", "memory");

  return result;
}

//: Write directly to memory
//
// Bypasses cache and other mechanisms
_INLINE_ void PIM_writeMem(unsigned int *addr, unsigned int data) {
  asm volatile ("lwz r3, %0\n" /* addr */
		"lwz r4, %1\n" /* data */
		"li r0, "SS_PIM_WRITE_MEM_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
                : 
  	        : "m" (addr), "m" (data)
		: "r0", "r3", "r4", "memory");
}

_INLINE_ unsigned int PIM_fastFree(void *ptr) 
{
  unsigned int size = 0;
  return PIM_fastFreeSize (ptr, size);
}

_INLINE_ unsigned int PIM_hwRand(void)
{
  unsigned int result;
  asm volatile ("li r0, "SS_PIM_RAND_STR"\n" /* Set to syscall */
		"sc\n" /*make the sys call*/
		"mr %0, r3\n" /*collect results*/
		: "=r" (result)
		:
		: "r0", "r3");
  return result;
}

_INLINE_ int PIM_quickPrint(unsigned int a, unsigned int b, unsigned int c)
{
 int result;
 asm volatile ("mr r3, %1\n" /* a */
	       "mr r4, %2\n" /* b */
	       "mr r5, %3\n" /* c */
	       "li r0, "SS_PIM_QUICK_PRINT_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
	       "mr %0, r3\n" /* Collect results*/
	       : "=r" (result)			
	       : "r" (a), "r" (b), "r" (c)
	       : "r0", "r3", "r4", "r5", "memory");
  return result;
}

_INLINE_ int PIM_trace(unsigned int a, unsigned int b, unsigned int c)
{
 int result;
 asm volatile ("mr r3, %1\n" /* a */
	       "mr r4, %2\n" /* b */
	       "mr r5, %3\n" /* c */
	       "li r0, "SS_PIM_TRACE_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
	       "mr %0, r3\n" /* Collect results*/
	       : "=r" (result)			
	       : "r" (a), "r" (b), "r" (c)
	       : "r0", "r3", "r4", "r5");
  return result;
}

/* This provides a sort of virtual-memory-esque trick. A good way to think
 * about it is as a remapping. One region of memory (starting at vstart and
 * continuing for size bytes) is remapped to another region (starting at kstart
 * and continuing for size bytes). This region is treated with the cache
 * characteristics indicated by cached.
 *
 * NOTE: This is specific to the processor. So, for example, in a PIM context,
 * only the PIM that ran this syscall will be aware of the mapping. */
_INLINE_ int PIM_mem_region_create(int region, void * vstart,
                unsigned long size, void * kstart, int cached)
{
 int result;
  asm volatile ("mr r3, %1\n" /* region */
               "mr r4, %2\n" /* vstart */
               "mr r5, %3\n" /* size */
               "mr r6, %4\n" /* kstart */
               "mr r7, %5\n" /* cached */
               "li r0, "SS_PIM_MEM_REGION_CREATE_STR"\n" /* Set to syscall */
               "sc\n" /* make the "system call" */
               "mr %0, r3\n" /* Collect results*/
               : "=r" (result)
                : "r" (region), "r" (vstart), "r" (size), "r" (kstart), "r" (cached)
                : "r0", "r3", "r4", "r5", "r6", "r7", "memory");
  return result;
}

_INLINE_ int PIM_mem_region_get(int region, unsigned long *addr,
                                        unsigned long *size )
{
 int result;
  asm volatile ("mr r3, %1\n" /* region */
               "mr r4, %2\n" /* addr */
               "mr r5, %3\n" /* size */
               "li r0, "SS_PIM_MEM_REGION_GET_STR"\n" /* Set to syscall */
               "sc\n" /* make the "system call" */
               "mr %0, r3\n" /* Collect results*/
               : "=r" (result)
                : "r" (region), "r" (addr), "r" (size)
                : "r0", "r3", "r4", "r5", "memory");
  return result;
}

_INLINE_ void PIM_writeSpecial(PIM_cmd c, unsigned int v1)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "li r0, "SS_PIM_WRITE_SPECIAL_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
	       : 
	       : "r" (c), "r" (v1)
	       : "r0", "r3", "r4");
}

_INLINE_ void PIM_writeSpecial2(PIM_cmd c, unsigned int v1, unsigned int v2)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "mr r5, %2\n" /* v2 */
	       "li r0, "SS_PIM_WRITE_SPECIAL2_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
	       : 
		: "r" (c), "r" (v1), "r" (v2)
		: "r0", "r3", "r4", "r5");
}

_INLINE_ void PIM_writeSpecial3(PIM_cmd c, unsigned int v1, unsigned int v2,
			      unsigned int v3)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "mr r5, %2\n" /* v2 */
	       "mr r6, %3\n" /* v3 */
	       "li r0, "SS_PIM_WRITE_SPECIAL3_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
		: 
		: "r" (c), "r" (v1), "r" (v2), "r" (v3)
		: "r0", "r3", "r4", "r5", "r6");
}

_INLINE_ void PIM_writeSpecial4(PIM_cmd c, unsigned int v1, unsigned int v2,
				unsigned int v3, unsigned int v4)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "mr r5, %2\n" /* v2 */
	       "mr r6, %3\n" /* v3 */
	       "mr r7, %4\n" /* v4 */
	       "li r0, "SS_PIM_WRITE_SPECIAL4_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
		: 
		: "r" (c), "r" (v1), "r" (v2), "r" (v3), "r" (v4)
		: "r0", "r3", "r4", "r5", "r6", "r7");
}

_INLINE_ void PIM_writeSpecial5(PIM_cmd c, unsigned int v1, unsigned int v2,
			      unsigned int v3, unsigned int v4, 
			      unsigned int v5)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "mr r5, %2\n" /* v2 */
	       "mr r6, %3\n" /* v3 */
	       "mr r7, %4\n" /* v4 */
	       "mr r8, %5\n" /* v5 */
	       "li r0, "SS_PIM_WRITE_SPECIAL5_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
		: 
		: "r" (c), "r" (v1), "r" (v2), "r" (v3), "r" (v4), "r" (v5)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8");
}

_INLINE_ void PIM_writeSpecial6(PIM_cmd c, unsigned int v1, unsigned int v2,
				unsigned int v3, unsigned int v4, 
				unsigned int v5, unsigned int v6)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "mr r5, %2\n" /* v2 */
	       "mr r6, %3\n" /* v3 */
	       "mr r7, %4\n" /* v4 */
	       "mr r8, %5\n" /* v5 */
	       "mr r9, %6\n" /* v6 */
	       "li r0, "SS_PIM_WRITE_SPECIAL6_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
		: 
		: "r" (c), "r" (v1), "r" (v2), "r" (v3), "r" (v4), "r" (v5), "r" (v6)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9");
}

_INLINE_ void PIM_writeSpecial7(PIM_cmd c, unsigned int v1, unsigned int v2,
				unsigned int v3, unsigned int v4, 
				unsigned int v5, unsigned int v6,
				unsigned int v7)
{
  asm volatile ("mr r3, %0\n" /* c */
	       "mr r4, %1\n" /* v1 */
	       "mr r5, %2\n" /* v2 */
	       "mr r6, %3\n" /* v3 */
	       "mr r7, %4\n" /* v4 */
	       "mr r8, %5\n" /* v5 */
	       "mr r9, %6\n" /* v6 */
	       "mr r10, %7\n" /* v7 */
	       "li r0, "SS_PIM_WRITE_SPECIAL7_STR"\n" /* Set to syscall */
	       "sc\n" /* make the "system call" */
		: 
		: "r" (c), "r" (v1), "r" (v2), "r" (v3), "r" (v4), "r" (v5), "r" (v6), "r" (v7)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10");
}

_INLINE_ int PIM_rwSpecial3(PIM_cmd c, unsigned int v1, unsigned int v2,
			  unsigned int v3)
{
  int result;
  asm volatile ("mr r3, %1\n" /* c */
		"mr r4, %2\n" /* v1 */
		"mr r5, %3\n" /* v2 */
		"mr r6, %4\n" /* v3 */
		"li r0, "SS_PIM_WRITE_SPECIAL3_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)
		: "r" (c), "r" (v1), "r" (v2), "r" (v3)
		: "r0", "r3", "r4", "r5", "r6", "memory");
  return result;
}

_INLINE_ int PIM_readSpecial(PIM_cmd c)
{
  int result;
  asm volatile ("mr r3, %1\n" /* c */
		"li r0, "SS_PIM_READ_SPECIAL_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)
		: "r" (c)
		: "r0", "r3", "memory");
  return result;
}

_INLINE_ int PIM_readSpecial1(PIM_cmd c, unsigned int v)
{
  int result;
  asm volatile ("mr r3, %1\n" /* c */
		"mr r4, %2\n" /* v */
		"li r0, "SS_PIM_READ_SPECIAL1_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)
		: "r" (c), "r" (v)
		: "r0", "r3", "r4", "memory");
  return result;
}

_INLINE_ int PIM_readSpecial2(PIM_cmd c, unsigned int v1, unsigned int v2)
{
  int result;
  asm volatile ("mr r3, %1\n" /* c */
		"mr r4, %2\n" /* v1 */
		"mr r5, %3\n" /* v2 */
		"li r0, "SS_PIM_READ_SPECIAL2_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)
		: "r" (c), "r" (v1), "r" (v2)
		: "r0", "r3", "r4", "r5", "memory");
  return result;
}

_INLINE_ int PIM_readSpecial3(PIM_cmd c, unsigned int v1, unsigned int v2, 
			    unsigned int v3)
{
  int result;
  asm volatile ("mr r3, %1\n" /* c */
		"mr r4, %2\n" /* v1 */
		"mr r5, %3\n" /* v2 */
		"mr r6, %4\n" /* v3 */
		"li r0, "SS_PIM_READ_SPECIAL3_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)
		: "r" (c), "r" (v1), "r" (v2), "r" (v3)
		: "r0", "r3", "r4", "r5", "r6", "memory");
  return result;
}

_INLINE_ int PIM_readSpecial4(PIM_cmd c, unsigned int v1, unsigned int v2, 
			    unsigned int v3, unsigned int v4)
{
  int result;
  asm volatile ("mr r3, %1\n" /* c */
		"mr r4, %2\n" /* v1 */
		"mr r5, %3\n" /* v2 */
		"mr r6, %4\n" /* v3 */
		"mr r7, %5\n" /* v3 */
		"li r0, "SS_PIM_READ_SPECIAL4_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)
		: "r" (c), "r" (v1), "r" (v2), "r" (v3), "r" (v4)
		: "r0", "r3", "r4", "r5", "r6", "r7", "memory");
  return result;
}

_INLINE_ int PIM_readSpecial1_2(PIM_cmd c, unsigned int v, unsigned int *o2)
{
  int result1;
  int result2;
  asm volatile ("mr r3, %2\n" /* c */
		"mr r4, %3\n" /* v */
		"li r0, "SS_PIM_READ_SPECIAL1_2_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		"mr %1, r4\n" /* Collect results*/
		: "=r" (result1), "=r" (result2)
		: "r" (c), "r" (v)
		: "r0", "r3", "r4", "memory");
  *o2 = result1;
  return result2;
}

_INLINE_ int PIM_readSpecial_2(PIM_cmd c, unsigned int *o2)
{
  int result1;
  int result2;
  asm volatile ("mr r3, %2\n" /* c */
		"li r0, "SS_PIM_READ_SPECIAL_2_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		"mr %1, r4\n" /* Collect results*/
		: "=r" (result1), "=r" (result2)
		: "r" (c)
		: "r0", "r3", "r4", "memory");
  *o2 = result1;
  return result2;
}

_INLINE_ int PIM_readSpecial1_5(PIM_cmd c, unsigned int v, unsigned int *o2,
			      unsigned int *o3, unsigned int *o4,
			      unsigned int *o5)
{
  int result1, result2, result3, result4, result5;
  asm volatile ("mr r3, %5\n" /* c */
		"mr r4, %6\n" /* v */
		"li r0, "SS_PIM_READ_SPECIAL1_5_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		"mr %1, r4\n" /* Collect results*/
		"mr %2, r5\n" /* Collect results*/
		"mr %3, r6\n" /* Collect results*/
		"mr %4, r7\n" /* Collect results*/
		: "=r" (result1), "=r" (result2), "=r" (result3), "=r" (result4), "=r" (result5)
		: "r" (c), "r" (v)
		: "r0", "r3", "r4", "r5", "r6", "r7", "memory");
  *o2 = result1;   *o3 = result2;  *o4 = result3;   *o5 = result4;
  return result5;
}

_INLINE_ void PIM_readSpecial1_6(PIM_cmd c, unsigned int v, unsigned int *o1,
			      unsigned int *o2, unsigned int *o3,
				unsigned int *o4, unsigned int *o5, 
				unsigned *o6)
{
  int result1, result2, result3, result4, result5, result6;
  asm volatile ("mr r3, %6\n" /* c */
		"mr r4, %7\n" /* v */
		"li r0, "SS_PIM_READ_SPECIAL1_6_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		"mr %1, r4\n" /* Collect results*/
		"mr %2, r5\n" /* Collect results*/
		"mr %3, r6\n" /* Collect results*/
		"mr %4, r7\n" /* Collect results*/
		"mr %5, r8\n" /* Collect results*/
		: "=r" (result1), "=r" (result2), "=r" (result3), "=r" (result4), "=r" (result5), "=r" (result6)
		: "r" (c), "r" (v)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "memory");
  *o1 = result1;   *o2 = result2;  *o3 = result3;   *o4 = result4; 
  *o5 = result5;   *o6 = result6;
}

_INLINE_ void PIM_readSpecial1_7(PIM_cmd c, unsigned int v, unsigned int *o1,
			      unsigned int *o2, unsigned int *o3,
				unsigned int *o4, unsigned int *o5, 
				 unsigned int *o6, unsigned int *o7)
{
  int result1, result2, result3, result4, result5, result6, result7;
  asm volatile ("mr r3, %7\n" /* c */
		"mr r4, %8\n" /* v */
		"li r0, "SS_PIM_READ_SPECIAL1_7_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		"mr %1, r4\n" /* Collect results*/
		"mr %2, r5\n" /* Collect results*/
		"mr %3, r6\n" /* Collect results*/
		"mr %4, r7\n" /* Collect results*/
		"mr %5, r8\n" /* Collect results*/
		"mr %6, r9\n" /* Collect results*/
		: "=r" (result1), "=r" (result2), "=r" (result3), "=r" (result4), "=r" (result5), "=r" (result6), "=r" (result7)
		: "r" (c), "r" (v)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "memory");
  *o1 = result1;   *o2 = result2;  *o3 = result3;   *o4 = result4; 
  *o5 = result5;   *o6 = result6; *o7 = result7;
}

#if defined(__cplusplus)
_INLINE_ void PIM_writeSpecial(PIM_cmd c, unsigned int v1, unsigned int v2) {
  PIM_writeSpecial2(c, v1, v2);
}
_INLINE_ void PIM_writeSpecial(PIM_cmd c, unsigned int v1, unsigned int v2, 
			     unsigned int v3) {
  PIM_writeSpecial3(c, v1, v2, v3);
}
_INLINE_ void PIM_writeSpecial(PIM_cmd c, unsigned int v1, unsigned int v2, 
			     unsigned int v3, unsigned int v4) {
  PIM_writeSpecial4(c, v1, v2, v3, v4);
}
_INLINE_ void PIM_writeSpecial(PIM_cmd c, unsigned int v1, unsigned int v2, 
			     unsigned int v3, unsigned int v4, 
			     unsigned int v5) {
  PIM_writeSpecial5(c, v1, v2, v3, v4, v5);
}
_INLINE_ int PIM_readSpecial(PIM_cmd c, unsigned int v) {
  return PIM_readSpecial1(c, v);
}
_INLINE_ int PIM_readSpecial(PIM_cmd c, unsigned int v1, unsigned int v2) {
  return PIM_readSpecial2(c, v1, v2);
}
_INLINE_ int PIM_readSpecial(PIM_cmd c, unsigned int v1, unsigned int v2,
			   unsigned int v3) {
  return PIM_readSpecial3(c, v1, v2, v3);
}
_INLINE_ int PIM_readSpecial(PIM_cmd c, unsigned int v1, unsigned int v2,
			   unsigned int v3, unsigned int v4) {
  return PIM_readSpecial4(c, v1, v2, v3, v4);
}
#endif

//: Switch between either PIM_feb_readfe or PIM_feb_writeef depending on the :defaultFEB
//
// The idea is that this allows things like pthread_mutex_lock to be
// implemented reliably regardless of simulator configuration changes,
// while still using the FEB subsystem. Essentially, this is identical to
// PIM_feb_writeef if :defaultFEB is 0, and is identical to PIM_feb_readfe
// if :defaultFEB is 1
_INLINE_ unsigned int PIM_feb_lock(unsigned int* a)
{
    unsigned int result;
    asm volatile ("mr r3, %1\n" /* a */
	          "li r0, "SS_PIM_LOCK_STR"\n" /* Set syscall */
		  "sc\n" /* make the "system call" */
		  "mr %0, r3\n" /* Collect results */
		  : "=r" (result)
		  : "r" (a)
		  : "r0", "r3", "memory");
    return result;
}

//: Switch between either PIM_feb_fill or PIM_feb_empty depending on the :defaultFEB
//
// The idea is that this allows things like pthread_mutex_unlock to be
// implemented reliably regardless of simulator configuration changes,
// while still using the FEB subsystem. Essentially, this is identical to
// PIM_feb_empty if :defaultFEB is 0, and is identical to PIM_feb_fill
// if :defaultFEB is 1
_INLINE_ unsigned int PIM_feb_unlock(unsigned int* a)
{
    unsigned int result;
    asm volatile ("mr r3, %1\n" /* a */
	          "li r0, "SS_PIM_UNLOCK_STR"\n" /* Set syscall */
		  "sc\n" /* make the "system call" */
		  "mr %0, r3\n" /* Collect results */
		  : "=r" (result)
		  : "r" (a)
		  : "r0", "r3", "memory");
    return result;
}

//: Read if FEB is full, set FEB to "full"
//
// Read a word at a specified address if the Full/Empty Bit (FEB)
// associated with that word is "full." After the read, keep the FEB
// to "full"
_INLINE_ unsigned int PIM_feb_readff(volatile unsigned int* a)
{
  unsigned int result;
  asm volatile ("mr r3, %1\n" /* a */
		"li r0, "SS_PIM_READFF_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)			
		: "r" (a)
		: "r0", "r3", "memory");
  return result;
}

//: Read if FEB is full, set FEB to "empty"
//
// Read a word at a specified address if the Full/Empty Bit (FEB)
// associated with that word is "full." After the read, set the FEB
// to "empty"
_INLINE_ unsigned int PIM_feb_readfe(volatile unsigned int* a)
{
  unsigned int result;
  asm volatile ("mr r3, %1\n" /* a */
		"li r0, "SS_PIM_READFE_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)			
		: "r" (a)
		: "r0", "r3", "memory");
  return result;
}

//: Atomically Increment integer at a given address
//
//!in: a: address of the integer to increment
_INLINE_ int PIM_atomicIncrement(volatile unsigned int* a, unsigned int i)
{
  int result;

  asm volatile ("mr r3, %1\n" /* a */
		"mr r4, %2\n" /* i */
		"li r0, "SS_PIM_ATOMIC_INCREMENT_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)			
		: "r" (a), "r" (i)
		: "r0", "r3", "r4", "memory");

  return result;
}

//: Write if FEB is empty, set FEB to "full"
//
// Write a word at the specified address if the FEB associated with
// that word is "empty," After the write, set the FEB to "full"
_INLINE_ void PIM_feb_writeef(volatile unsigned int* a, unsigned int v)
{
  asm volatile ("mr r3, %0\n" /* a */
		"mr r4, %1\n" /* v */
		"li r0, "SS_PIM_WRITEEF_STR"\n"/* Set syscall */
		"sc\n" /* make the "system call" */
		:
		: "r" (a), "r" (v)
		: "r0", "r3", "r4", "memory");
}




//: Set a Full/Empty bit to "full"
//
// Set the FEB associated with the given address to "full" - do not
// change the data contents of the associated address.
_INLINE_ void PIM_feb_fill(unsigned int* a) {
  asm volatile ("mr r3, %0\n" /* a */
		"li r0, "SS_PIM_FILL_FE_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		: 
		: "r" (a)
		: "r0", "r3", "memory");
}

//: Set a Full/Empty bit to "empty"
//
// Set the FEB associated with the given address to "empty" - do not
// change the data contents of the associated address.
_INLINE_ void PIM_feb_empty(unsigned int* a) {
  asm volatile ("mr r3, %0\n" /* a */
		"li r0, "SS_PIM_EMPTY_FE_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		: 
		: "r" (a)
		: "r0", "r3", "memory");
}

//: Set a Full/Empty bit to "empty"
//
// Set the FEB associated with the given address to "empty" - do not
// change the data contents of the associated address.
//
// This is the same as PIM_feb_empty(), it is provided to remain
// consistent with Cray terminology.
_INLINE_ void PIM_feb_purge(unsigned int* a)
{
  PIM_feb_empty(a);
}

//: Check the fullness of a FEB
//
// Check is the FEB assoicated with a specified memory address is set
// to "full"(1) or "empty"(0).
_INLINE_ int PIM_feb_is_full(unsigned int* a)
{
  int result;
  asm volatile ("mr r3, %1\n" /* a */
		"li r0, "SS_PIM_IS_FE_FULL_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)			
		: "r" (a)
		: "r0", "r3");
  return result;
}

//: Try to lock a FEB
//
// Attempt to move given FEB from empty to full. If it is already
// full, returns 1, else returns 0.
_INLINE_ int PIM_feb_tryef(unsigned int* a)
{
  int result;
  asm volatile ("mr r3, %1\n" /* a */
		"li r0, "SS_PIM_TRYEF_STR"\n"/*Set syscall*/
		"sc\n" /* make the "system call" */
		"mr %0, r3\n" /* Collect results*/
		: "=r" (result)			
		: "r" (a)
		: "r0", "r3", "memory");
  return result;
}

#ifdef __cplusplus

//: Read if FEB is full, set FEB to "full"
//
// Read a word at a specified address if the Full/Empty Bit (FEB)
// associated with that word is "full." After the read, keep the FEB
// to "full"
//
// This is a templatized version of PIM_feb_readff()
template <class T>
_INLINE_ T PIM_readff(volatile T* a) {
  return (T)PIM_feb_readff((volatile unsigned int*)a);
}

//: Read if FEB is full, set FEB to "empty"
//
// Read a word at a specified address if the Full/Empty Bit (FEB)
// associated with that word is "full." After the read, set the FEB
// to "empty"
//
// This is a templatized version of PIM_feb_readfe()
template <class T>
_INLINE_ T PIM_readfe(volatile T* a) {
  return (T)PIM_feb_readfe((volatile unsigned int*)a);
}

//: Write if FEB is empty, set FEB to "full"
//
// Write a word at the specified address if the FEB associated with
// that word is "empty," After the write, set the FEB to "full"
//
// This is a templatized version of PIM_feb_writeef()
template <class T>
_INLINE_ void PIM_writeef(volatile T* a, T v) {
  PIM_feb_writeef((volatile unsigned int*)a, (unsigned int)v);
}

//: Set a Full/Empty bit to "full"
//
// Set the FEB associated with the given address to "full" - do not
// change the data contents of the associated address.
//
// This is a templatized version of PIM_feb_fill()
template <class T>
_INLINE_ void PIM_fill(T* a) {
  PIM_feb_fill((unsigned int*)a);
}

//: Set a Full/Empty bit to "empty"
//
// Set the FEB associated with the given address to "empty" - do not
// change the data contents of the associated address.
//
// This is a templatized version of PIM_feb_empty()
template <class T>
_INLINE_ void PIM_empty(T* a) {
  PIM_feb_empty((unsigned int*) a);
}

//: Set a Full/Empty bit to "empty"
//
// Set the FEB associated with the given address to "empty" - do not
// change the data contents of the associated address.
//
// This is the same as PIM_empty(), it is provided to remain
// consistent with Cray terminology.
template <class T>
_INLINE_ int PIM_purge(T* a) {
  return PIM_feb_purge((unsigned int*)a);
}

//: Check the fullness of a FEB
//
// Check is the FEB assoicated with a specified memory address is set
// to "full"(1) or "empty"(0).
//
// This is a templatized version of PIM_feb_empty()
template <class T>
_INLINE_ int PIM_is_full(T* a) {
  return PIM_feb_is_full((unsigned int*)a);
}
#endif


//:Reset performance counters
//
// Resets counts of instructions, cycles, and migrations.
_INLINE_ void PIM_resetCounters(void)
{
  asm volatile ("li r0, "SS_PIM_RESET_STR"\n"/* Set syscall */
		"sc\n" /* make the "system call" */
                : 
  	        : 
		: "r0");
}

//:Terminate a PIM thread
//
// Wrapper around the PIM system call.
_INLINE_ void PIM_threadExit(void) __attribute__((noreturn));
_INLINE_ void PIM_threadExit(void)
{
  asm volatile ("li r0, "SS_PIM_EXIT_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
                : : : "r0");
/* NOTE: this function is marked (noreturn), and the compiler ordinarily
 * complains that this function falls off the bottom. BUT, to suppress this
 * warning, we've added this infinite-loop to the end of the function. The
 * compiler is smart enough to realize that this prevents the function from
 * ever returning (for a minimum of instructions wasted), but unfortunately
 * forces the compiler to add dead code to the function. Sorry :-( */
  for (;;) continue;
}

/* this makes sure the stack is freed */
_INLINE_ void PIM_threadExitFree(void) __attribute__((__noreturn__));
_INLINE_ void PIM_threadExitFree(void)
{
  asm volatile ("li r0, "SS_PIM_EXIT_FREE_STR"\n" /* Set to syscall */
		"sc\n" /* make the "system call" */
                : : : "r0");
/* NOTE: this function is marked (noreturn), and the compiler ordinarily
 * complains that this function falls off the bottom. BUT, to suppress this
 * warning, we've added this infinite-loop to the end of the function. The
 * compiler is smart enough to realize that this prevents the function from
 * ever returning (for a minimum of instructions wasted), but unfortunately
 * forces the compiler to add dead code to the function. Sorry :-( */
  for (;;) continue;
}

#endif
