/*
** This is the stuff we need on x86 from Apple's sysctl.h
*/
#ifndef _PPC_SYS_SYSCTL_H_
#define	_PPC_SYS_SYSCTL_H_

/*
** Define these so the native sysctl stuff does not get included later
*/
#define _LINUX_SYSCTL_H
#define _SYS_SYSCTL_H

/*
 * Top-level identifiers
 */
#define	CTL_UNSPEC	0		/* unused */
#define	CTL_KERN	1		/* "high kernel": proc, limits */
#define	CTL_VM		2		/* virtual memory */
#define	CTL_VFS		3		/* file system, mount type is next */
#define	CTL_NET		4		/* network, see socket.h */
#define	CTL_DEBUG	5		/* debugging parameters */
#define	CTL_HW		6		/* generic cpu/io */
#define	CTL_MACHDEP	7		/* machine dependent */
#define	CTL_USER	8		/* user-level */
#define	CTL_MAXID	9		/* number of valid top-level ids */


/*
 * CTL_HW identifiers
 */
#define	HW_MACHINE	 1		/* string: machine class */
#define	HW_MODEL	 2		/* string: specific machine model */
#define	HW_NCPU		 3		/* int: number of cpus */
#define	HW_BYTEORDER	 4		/* int: machine byte order */
#define	HW_PHYSMEM	 5		/* int: total memory */
#define	HW_USERMEM	 6		/* int: non-kernel memory */
#define	HW_PAGESIZE	 7		/* int: software page size */
#define	HW_DISKNAMES	 8		/* strings: disk drive names */
#define	HW_DISKSTATS	 9		/* struct: diskstats[] */
#define	HW_EPOCH  	10		/* int: 0 for Legacy, else NewWorld */
#define HW_FLOATINGPT	11		/* int: has HW floating point? */
#define HW_MACHINE_ARCH	12		/* string: machine architecture */
#define HW_VECTORUNIT	13		/* int: has HW vector unit? */
#define HW_BUS_FREQ	14		/* int: Bus Frequency */
#define HW_CPU_FREQ	15		/* int: CPU Frequency */
#define HW_CACHELINE	16		/* int: Cache Line Size in Bytes */
#define HW_L1ICACHESIZE	17		/* int: L1 I Cache Size in Bytes */
#define HW_L1DCACHESIZE	18		/* int: L1 D Cache Size in Bytes */
#define HW_L2SETTINGS	19		/* int: L2 Cache Settings */
#define HW_L2CACHESIZE	20		/* int: L2 Cache Size in Bytes */
#define HW_L3SETTINGS	21		/* int: L3 Cache Settings */
#define HW_L3CACHESIZE	22		/* int: L3 Cache Size in Bytes */
#define HW_TB_FREQ	23		/* int: Bus Frequency */
#define HW_MEMSIZE	24		/* uint64_t: physical ram size */
#define HW_AVAILCPU	25		/* int: number of available CPUs */
#define	HW_MAXID	26		/* number of valid hw ids */

#ifdef __cplusplus
extern "C" {
#endif
int	sysctl(int *, u_int, void *, size_t *, void *, size_t);
int	sysctlbyname(const char *, void *, size_t *, void *, size_t);
int	sysctlnametomib(const char *, int *, size_t *);

#ifdef __cplusplus
}
#endif

#endif	/* !_PPC_SYS_SYSCTL_H_ */
