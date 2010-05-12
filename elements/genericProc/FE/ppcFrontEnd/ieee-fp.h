/*
 * Copyright (C) 1992,1995 by
 * Digital Equipment Corporation, Maynard, Massachusetts.
 * This file may be redistributed according to the terms of the
 * GNU General Public License.
 */

#ifndef __ieee_math_h__
#define __ieee_math_h__


extern void div128( unsigned long long a[2], unsigned long long b[2],unsigned long long c[2]);

extern void ieee_ADDT (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_ADDS (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_SUBT (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_SUBS (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_DIVS (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_MULS (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_DIVT (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);
extern void ieee_MULT (int rm, unsigned long long a, unsigned long long b,
				unsigned long long *c, unsigned int *pi);

extern void
ieee_CVTW (int f, unsigned long long a, unsigned long long *b, unsigned int *pi);
extern void
ieee_CVTTS (int f, unsigned long long a, unsigned long long *b, unsigned int *pi);

extern void 
setFPSCRFPRFFlags(unsigned long long b, unsigned int *pi);

extern void 
negate(unsigned long long a, unsigned long long *b, unsigned int *pi);

#endif /* __ieee_math_h__ */


