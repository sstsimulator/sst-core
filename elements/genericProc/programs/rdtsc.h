
#include <sys/types.h>

#define TBL 0x10c
#define TBU 0x10d

#define mfspr(reg,var) __asm__ __volatile__ \
        ("mfspr %0, %1" : "=r" (var) : "i" (reg) )

static __inline__ u_int64_t _rdtsc(void)
{
        u_int32_t upper;
        u_int32_t lower;

        mfspr(TBU,upper);
        mfspr(TBL,lower);

        return ( (u_int64_t)upper<<32 | lower);
}

