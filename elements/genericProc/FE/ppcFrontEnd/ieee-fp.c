/*
 * ieee-math.c - IEEE floating point emulation code
 * Copyright (C) 1989,1990,1991,1995 by
 * Digital Equipment Corporation, Maynard, Massachusetts.
 *
 * Heavily modified for Linux/Alpha.  Changes are Copyright (c) 1995
 * by David Mosberger (davidm@azstarnet.com).
 *
 * This file may be redistributed according to the terms of the
 * GNU General Public License.
 */
/*
 * The original code did not have any comments. I have created many
 * comments as I fix the bugs in the code.  My comments are based on
 * my observation and interpretation of the code.  If the original
 * author would have spend a few minutes to comment the code, we would
 * never had a problem of misinterpretation.  -HA
 *
 * This code could probably be a lot more optimized (especially the
 * division routine).  However, my foremost concern was to get the
 * IEEE behavior right.  Performance is less critical as these
 * functions are used on exceptional numbers only (well, assuming you
 * don't turn on the "trap on inexact"...).
 */
/*
 *This file has been modified for use with SimpleScalar to emulate PowerPC floating point 
 *arithemtic.
 *
 *
 */

#include "ieee-fp.h"
#define STICKY_S	0x20000000	/* both in longword 0 of fraction */
#define STICKY_T	1
#include <stdio.h>
#include <math.h>


#define ROUND_CHOP 	(0x0)
#define ROUND_NINF 	(0x1)
#define ROUND_NEAR 	(0x2)
#define ROUND_PINF      (0x3)
#define POSINFX(E)         (((E&0xfff00000)==0x7ff00000)&((E&0x000fffff)==0))
#define NEGINFX(E)         (((E&0xfff00000)==0xfff00000)&((E&0x000fffff)==0))

#define POSINFSPX(E)         (((E&0xff800000)==0x7f800000)&((E&0x000fffff)==0))
#define NEGINFSPX(E)         (((E&0xff800000)==0xff800000)&((E&0x000fffff)==0))



/*
 * Careful: order matters here!
 */
enum {
	NaN, QNaN, INFTY, ZERO, DENORM, NORMAL
};

enum {
	SINGLE, DOUBLE
};

typedef unsigned long fpclass_t;

#define IEEE_TMAX	0x7fefffffffffffffll

#define IEEE_SMAX       0x47efffffe0000000ll
#define IEEE_SNaN       0xfff00000000f0000ll
#define IEEE_QNaN       0xfff8000000000000ll
#define IEEE_PINF       0x7ff0000000000000ll
#define IEEE_NINF       0xfff0000000000000ll


/*Macros used in manipulating the FPSCR registers*/
#define GET_FPSCR_UE_VAL(E)         ((E&0x20)>>5)
#define GET_FPSCR_OE_VAL(E)         ((E&0x40)>>6)
#define GET_FPSCR_VE_VAL(E)         ((E&0x80)>>7)
#define SET_FPSCR_FX_VAL(E)         (E=(E|0x8000000000000000ll))
#define SET_FPSCR_UX_VAL(E)         (E=(E|0x0800000000000000ll))
#define SET_FPSCR_OX_VAL(E)         (E=(E|0x1000000000000000ll))
#define RESET_FPSCR_FR_VAL(E)       (E=(E&0xfffbffffffffffffll))
#define RESET_FPSCR_FI_VAL(E)       (E=(E&0xfffdffffffffffffll))
#define SET_FPSCR_FR_VAL(E)         (E=(E|0x0004000000000000ll))
#define SET_FPSCR_FI_VAL(E)         (E=(E|0x0002000000000000ll))
#define SET_FPSCR_XX_VAL(E)         (E=(E|0x0200000000000000ll))
#define SET_FPSCR_VXISI_VAL(E)      (E=(E|0x0080000000000000ll)) 
#define SET_FPSCR_VXSNAN_VAL(E)     (E=(E|0x0100000000000000ll)) 
#define SET_FPSCR_VXIMZ_VAL(E)      (E=(E|0x0010000000000000ll)) 
#define SET_FPSCR_VXIDI_VAL(E)      (E=(E|0x0040000000000000ll)) 
#define SET_FPSCR_VXCVI_VAL(E)      (E=(E|0x0000000000000100ll)) 
#define SET_FPSCR_FPRF_VAL(E,V)     (E=((E&0xfffffffffffe0fffll)|((V&0x1f)<<12)))




/*
 * The memory format of S floating point numbers differs from the
 * register format.  In the following, the bitnumbers above the
 * diagram below give the memory format while the numbers below give
 * the register format.
 *
 *	  31 30	     23 22				0
 *	+-----------------------------------------------+
 * S	| s |	exp    |       fraction			|
 *	+-----------------------------------------------+
 *	  63 62	     52 51			      29
 *	
 * For T floating point numbers, the register and memory formats
 * match:
 *
 *	+-------------------------------------------------------------------+
 * T	| s |	     exp	|	    frac | tion			    |
 *	+-------------------------------------------------------------------+
 *	  63 62		      52 51	       32 31			   0
 */
typedef struct {
	unsigned long long f[2];	/* bit 55 in f is the factor of 2^0*/
	int		s;	/* 1 bit sign (0 for +, 1 for -) */
	int		e;	/* 16 bit signed exponent */
} EXTENDED;


/*
 * Return the sign of a Q integer, S or T fp number in the register
 * format.
 */
static inline int
sign (unsigned long long a)
{
	if ((long long) a < 0)
		return -1;
	else
		return 1;
}


static inline long long
cmp128 (const unsigned long long a[2], const unsigned long long b[2])
{
	if (a[1] < b[1]) return -1;
	if (a[1] > b[1]) return  1;
	return a[0] - b[0];
}


static inline void
sll128 (unsigned long long a[2])
{
	a[1] = (a[1] << 1) | (a[0] >> 63);
	a[0] <<= 1;
}


static inline void
srl128 (unsigned long long a[2])
{
	a[0] = (a[0] >> 1) | (a[1] << 63);
	a[1] >>= 1;
}


static inline void
add128 (const unsigned long long a[2], const unsigned long long b[2], unsigned long long c[2])
{
	unsigned long long carry = a[0] > (0xffffffffffffffffll - b[0]);

	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1] + carry;
}


static inline void
sub128 (const unsigned long long a[2], const unsigned long long b[2], unsigned long long c[2])
{
	unsigned long long borrow = a[0] < b[0];

	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1] - borrow;
}

void
mul64 (const unsigned long long _la, const unsigned long long _lb, unsigned long long c[2])
{
  unsigned long long a,b, ta[2], tc[2];
  unsigned long long count,i;
  
  /*
	asm ("mulq  %2,%3,%0\n\t"
	     "umulh %2,%3,%1"
	     : "r="(c[0]), "r="(c[1]) : "r"(a), "r"(b));
  */
  /* Some code here to do the above*/
  if (_lb>_la) {
    a=_lb;
    b=_la;
  } 
  else{
    a=_la;
    b=_lb;
  }
  
  c[0]=c[1]=0;

  if(b==0) return;

  c[0]=ta[0]=a;

  ta[1]=0;

  count = (unsigned long long)(log(b)/log(2));
  for(i=0;i<count;i++){
    sll128(c);
  }

  tc[0]=c[0];
  tc[1]=c[1];

  count = b - (unsigned long long)pow((double)2,(double)count);
  for(i=0;i<count;i++){
    add128(tc,ta,c);
    tc[0]=c[0];
    tc[1]=c[1];
  }

}


void
div128 (unsigned long long a[2], unsigned long long b[2], unsigned long long c[2])
{
	unsigned long long mask[2] = {1, 0};

	/*
	 * Shift b until either the sign bit is set or until it is at
	 * least as big as the dividend:
	 */
	while (cmp128(b, a) < 0 && sign(b[1]) >= 0) {
		sll128(b);
		sll128(mask);
	}
	c[0] = c[1] = 0;
	do {
		if (cmp128(a, b) >= 0) {
			sub128(a, b, a);
			add128(mask, c, c);
		}
		srl128(mask);
		srl128(b);
	} while (mask[0] || mask[1]);
}


static void
normalize (EXTENDED *a)
{
	if (!a->f[0] && !a->f[1])
		return;		/* zero fraction, unnormalizable... */
	/*
	 * In "extended" format, the "1" in "1.f" is explicit; it is
	 * in bit 55 of f[0], and the decimal point is understood to
	 * be between bit 55 and bit 54.  To normalize, shift the
	 * fraction until we have a "1" in bit 55.
	 */
	if ((a->f[0] & 0xff00000000000000ll) != 0 || a->f[1] != 0) {
		/*
		 * Mantissa is greater than 1.0:
		 */
		while ((a->f[0] & 0xff80000000000000ll) != 0x0080000000000000ll ||
		       a->f[1] != 0)
		{
			unsigned long long sticky;

			++a->e;
			sticky = a->f[0] & 1;
			srl128(a->f);
			a->f[0] |= sticky;
		}
		return;
	}

	if (!(a->f[0] & 0x0080000000000000ll)) {
		/*
		 * Mantissa is less than 1.0:
		 */
		while (!(a->f[0] & 0x0080000000000000ll)) {
			--a->e;
			a->f[0] <<= 1;
		}
		return;
	}
}

static inline fpclass_t
ieee_fpclass (unsigned long long a)
{
	unsigned long long exp, fract;

	exp   = (a >> 52) & 0x7ff;	/* 11 bits of exponent */
	fract = a & 0x000fffffffffffffll;	/* 52 bits of fraction */
	if (exp == 0) {
		if (fract == 0)
			return ZERO;
		return DENORM;
	}
	if (exp == 0x7ff) {
		if (fract == 0)
			return INFTY;
		if (((fract >> 51) & 1) != 0)
			return QNaN;
		return NaN;
	}
	return NORMAL;
}


/*
 * Translate S/T fp number in register format into extended format.
 */

static fpclass_t
extend_ieee (unsigned long long a, EXTENDED *b, int prec)
{
	fpclass_t result_kind;

	b->s = a >> 63;
	b->e = ((a >> 52) & 0x7ff) - 0x3ff;	/* remove bias */
	b->f[1] = 0;
	/*
	 * We shift f[1] left three bits so that the higher order bits
	 * of the fraction will reside in bits 55 through 0 of f[0].
	 */
	b->f[0] = (a & 0x000fffffffffffffll) << 3;
	result_kind = ieee_fpclass(a);
	if (result_kind == NORMAL) {
		/* set implied 1. bit: */
		b->f[0] |= ((unsigned long long)0x1) << 55;
	} else if (result_kind == DENORM) {
		if (prec == SINGLE)
			b->e = -126;
		else
			b->e = -1022;
	}
	return result_kind;
}

void
make_s_ieee (long f, EXTENDED *a, unsigned long long *b, unsigned int *pi)
{
  unsigned long long sticky;
  if (!a->f[0] && !a->f[1]) {
    *b = (unsigned long long) a->s << 63;	/* return +/-0 */
    if(a->s) SET_FPSCR_FPRF_VAL(*pi,0x12);
    else SET_FPSCR_FPRF_VAL(*pi,0x2);
    return ;
  }
  

  normalize(a);
  
  
  if (a->e < -0x7e) {
   if (GET_FPSCR_UE_VAL(*pi)) {
     SET_FPSCR_UX_VAL(*pi);
     SET_FPSCR_FX_VAL(*pi);
     
     a->e += 0xc0;	/* scale up result by 2^alpha */
   } else {
     
     /* try making denormalized number: */
      while (a->e < -0x7e) {
	++a->e;
	sticky = a->f[0] & 1;
	srl128(a->f);
	if (!a->f[0] && !a->f[0]) {
	  /* underflow: replace with exact 0 */
	  
	  break;
	}
	a->f[0] |= sticky;
      }
      a->e = -0x3ff;
    }
  }
  
  if (a->e >= 0x80) {
    if (GET_FPSCR_OE_VAL(*pi)) {
      SET_FPSCR_OX_VAL(*pi);
      SET_FPSCR_FX_VAL(*pi);
      
      a->e -= 0xc0;	/* scale down result by 2^alpha */
    } else {
      
      /*
       * Overflow, substitute
       * result according to rounding mode:
       */
      switch (f) {
      case ROUND_NEAR:
	*b = IEEE_PINF;
	break;
	
      case ROUND_CHOP:
	*b = IEEE_SMAX;
      break;
      
      case ROUND_NINF:
	if (a->s) {
	  *b = IEEE_PINF;
	} else {
	  *b = IEEE_SMAX;
	}
	break;
	
      case ROUND_PINF: 
	if (a->s) {
	  *b = IEEE_SMAX;
	} else {
	  *b = IEEE_PINF;
	}
	break;
      }
      *b |= ((unsigned long long) a->s << 63);
      return;
    }
  }
  
  *b = (((unsigned long long) a->s << 63) |
	(((unsigned long long) a->e + 0x3ff) << 52) |
	((a->f[0] >> 3) & 0x000fffffe0000000ll));
  return ;
}


void
make_t_ieee (long f, EXTENDED *a, unsigned long long *b, unsigned int *pi)
{
  unsigned long long sticky;

  if (!a->f[0] && !a->f[1]) {
    *b = (unsigned long long) a->s << 63;	/* return +/-0 */
    if(a->s) SET_FPSCR_FPRF_VAL(*pi,0x12);
    else SET_FPSCR_FPRF_VAL(*pi,0x2);
    return ;
  }
  
  
  normalize(a);
  
  if (a->e < -0x3fe) {

    if (GET_FPSCR_UE_VAL(*pi)) {
     
      SET_FPSCR_UX_VAL(*pi);
      SET_FPSCR_FX_VAL(*pi);
  
      a->e += 0x600;
    } 
    else {
      
    /* try making denormalized number: */
      while (a->e < -0x3fe) {
	++a->e;
	sticky = a->f[0] & 1;
	
	srl128(a->f);
	if (!a->f[0]&&!a->f[1]) {
	  
	  break;
	}
	
	a->f[0] |= sticky;
      }
      a->e = -0x3ff;
    }       
  }
  if (a->e > 0x3ff) {

    if (GET_FPSCR_OE_VAL(*pi)) {
      a->e -= 0x600;	/* scale down result by 2^alpha */
      SET_FPSCR_OX_VAL(*pi);
      SET_FPSCR_FX_VAL(*pi);

    } else {
      
      /*
       * Overflow , substitute
       * result according to rounding mode:
       */
      switch (f) {
      case ROUND_NEAR:
	*b = IEEE_PINF;
	break;
	
      case ROUND_CHOP:
	*b = IEEE_TMAX;
	break;
	
      case ROUND_NINF:
	if (a->s) {
	*b = IEEE_PINF;
	} else {
	  *b = IEEE_TMAX;
	}
	break;
	
      case ROUND_PINF: 
	if (a->s) {
	  *b = IEEE_TMAX;
	} else {
	  *b = IEEE_PINF;
	}
	break;
      }
      *b |= ((unsigned long long) a->s << 63);
      return;
    }
  }
  
  *b = (((unsigned long long) a->s << 63) |
	(((unsigned long long) a->e + 0x3ff) << 52) |
	((a->f[0] >> 3) & 0x000fffffffffffffll));
  return;
}


void
round_s_ieee (int f, EXTENDED *a, unsigned long long *b, unsigned int *pi )
{
	unsigned long long diff1, diff2;
	EXTENDED z1, z2;

	if (!(a->f[0] & 0xffffffff)) {
	  RESET_FPSCR_FR_VAL(*pi);
	  RESET_FPSCR_FI_VAL(*pi);
	  make_s_ieee(f, a, b, pi);	/* no rounding error */
	  return;
	}
	SET_FPSCR_FR_VAL(*pi);
	SET_FPSCR_FI_VAL(*pi);
	SET_FPSCR_XX_VAL(*pi);
	SET_FPSCR_FX_VAL(*pi);

	/*
	 * z1 and z2 are the S-floating numbers with the next smaller/greater
	 * magnitude than a, respectively.
	 */
	z1.s = z2.s = a->s;
	z1.e = z2.e = a->e;
	z1.f[0] = z2.f[0] = a->f[0] & 0xffffffff00000000ll;
	z1.f[1] = z2.f[1] = 0;
	z2.f[0] += 0x100000000ll;	/* next bigger S float number */

	switch (f) {
	      case ROUND_NEAR:
		diff1 = a->f[0] - z1.f[0];
		diff2 = z2.f[0] - a->f[0];
		if (diff1 > diff2)
			 make_s_ieee(f, &z2, b, pi);
		else if (diff2 > diff1)
			 make_s_ieee(f, &z1, b, pi);
		else
			/* equal distance: round towards even */
			if (z1.f[0] & 0x100000000ll)
				 make_s_ieee(f, &z2, b, pi);
			else
				 make_s_ieee(f, &z1, b, pi);
		break;

	      case ROUND_CHOP:
		 make_s_ieee(f, &z1, b, pi);
		break;

	      case ROUND_PINF:
		if (a->s) {
			 make_s_ieee(f, &z1, b, pi);
		} else {
			 make_s_ieee(f, &z2, b, pi);
		}
		break;

	      case ROUND_NINF:
		if (a->s) {
			 make_s_ieee(f, &z2, b, pi);
		} else {
			 make_s_ieee(f, &z1, b, pi);
		}
		break;
	}
	return ;
}

void
round_t_ieee (int f, EXTENDED *a, unsigned long long *b, unsigned int *pi)
{
	unsigned long diff1, diff2, res;
	EXTENDED z1, z2;

	if (!(a->f[0] & 0x7)) {
		/* no rounding error */
	  RESET_FPSCR_FR_VAL(*pi);
	  RESET_FPSCR_FI_VAL(*pi);
	  make_t_ieee(f, a, b, pi);
	  return;
	}

	SET_FPSCR_FR_VAL(*pi);
	SET_FPSCR_FI_VAL(*pi);
	SET_FPSCR_XX_VAL(*pi);
	SET_FPSCR_FX_VAL(*pi);

	z1.s = z2.s = a->s;
	z1.e = z2.e = a->e;
	z1.f[0] = z2.f[0] = a->f[0] & ~0x7;
	z1.f[1] = z2.f[1] = 0;
	z2.f[0] += (1 << 3);

	res = 0;
	switch (f) {
	      case ROUND_NEAR:
		diff1 = a->f[0] - z1.f[0];
		diff2 = z2.f[0] - a->f[0];
		if (diff1 > diff2)
			 make_t_ieee(f, &z2, b,pi);
		else if (diff2 > diff1)
			make_t_ieee(f, &z1, b,pi);
		else
			/* equal distance: round towards even */
			if (z1.f[0] & (1 << 3))
				 make_t_ieee(f, &z2, b,pi);
			else
				 make_t_ieee(f, &z1, b,pi);
		break;

	      case ROUND_CHOP:
		make_t_ieee(f, &z1, b,pi);
		break;

	      case ROUND_PINF:
		if (a->s) {
		         make_t_ieee(f, &z1, b,pi);
		} else {
			 make_t_ieee(f, &z2, b,pi);
		}
		break;

	      case ROUND_NINF:
		if (a->s) {
			 make_t_ieee(f, &z2, b,pi);
		} else {
			 make_t_ieee(f, &z1, b,pi);
		}
		break;
	}
	return;
}


void
add_kernel_ieee (EXTENDED *op_a, EXTENDED *op_b, EXTENDED *op_c)
{
	unsigned long long mask, fa, fb, fc;
	int diff;

	diff = op_a->e - op_b->e;
	fa = op_a->f[0];
	fb = op_b->f[0];
	if (diff < 0) {
		diff = -diff;
		op_c->e = op_b->e;
		mask = ( ((unsigned long long)0x1)<< diff) - 1;
		fa >>= diff;
		if (op_a->f[0] & mask) {
			fa |= 1;		/* set sticky bit */
		}
	} else {
		op_c->e = op_a->e;
		mask = (((unsigned long long)0x1) << diff) - 1;
		fb >>= diff;
		if (op_b->f[0] & mask) {
			fb |= 1;		/* set sticky bit */
		}
	}
	if (op_a->s)
		fa = -fa;
	if (op_b->s)
		fb = -fb;
	fc = fa + fb;
	op_c->f[1] = 0;
	op_c->s = fc >> 63;
	if (op_c->s) {
		fc = -fc;
	}
	op_c->f[0] = fc;
	normalize(op_c);
	return;
}





/*
 * Add a + b = c, where a, b, and c are ieee t-floating numbers.  "f"
 * contains the rounding mode etc.
 */
void
ieee_ADDT (int f, unsigned long long  a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	a_type = extend_ieee(a, &op_a, DOUBLE);
	b_type = extend_ieee(b, &op_b, DOUBLE);

	if((POSINFX(a)&&NEGINFX(b))||(POSINFX(b)&&NEGINFX(a)))
	  {
	    SET_FPSCR_VXISI_VAL(*pi);SET_FPSCR_FX_VAL(*pi);
	  }
	if( a_type==NaN||b_type==NaN) 
	  {
	  SET_FPSCR_VXSNAN_VAL(*pi);SET_FPSCR_FX_VAL(*pi);
	  }


	
	if ((a_type >= NaN && a_type <= INFTY) ||
	    (b_type >= NaN && b_type <= INFTY))
	{
		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | (((unsigned long long)0x1) << 51);

		if (a_type == NaN || b_type == NaN)
			return;
		if (a_type == QNaN || b_type == QNaN)
			return;

		if (a_type == INFTY && b_type == INFTY && sign(a) != sign(b)) {
			*c = IEEE_QNaN;
			return;
		}
		if (a_type == INFTY)
			*c = a;
		else
			*c = b;
		return ;
	}
	add_kernel_ieee(&op_a, &op_b, &op_c);
	/* special case for -0 + -0 ==> -0 */
	if (a_type == ZERO && b_type == ZERO)
		op_c.s = op_a.s && op_b.s;

	round_t_ieee(f, &op_c, c,pi);
}


/*
 * Add a + b = c, where a, b, and c are ieee s-floating numbers.  "f"
 * contains the rounding mode etc.
 */
void
ieee_ADDS (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	a_type = extend_ieee(a, &op_a, SINGLE);
	b_type = extend_ieee(b, &op_b, SINGLE);

	if((POSINFSPX(a)&&NEGINFSPX(b))||(POSINFSPX(b)&&NEGINFSPX(a))){
	  SET_FPSCR_VXISI_VAL(*pi);SET_FPSCR_FX_VAL(*pi);
	}
	  
	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}



	if ((a_type >= NaN && a_type <= INFTY) ||
	    (b_type >= NaN && b_type <= INFTY))
	{
		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | (((unsigned long long)0x1) << 51);


		if (a_type == NaN || b_type == NaN)
			return;
		if (a_type == QNaN || b_type == QNaN)
			return;

		if (a_type == INFTY && b_type == INFTY && sign(a) != sign(b)) {
			*c = IEEE_QNaN;
			return;
		}
		if (a_type == INFTY)
			*c = a;
		else
			*c = b;
		return;
	}

	add_kernel_ieee(&op_a, &op_b, &op_c);
	/* special case for -0 + -0 ==> -0 */
	if (a_type == ZERO && b_type == ZERO)
		op_c.s = op_a.s && op_b.s;
	round_s_ieee(f, &op_c, c, pi);
}




/*
 * Subtract a - b = c, where a, b, and c are ieee t-floating numbers.
 * "f" contains the rounding mode etc.
 */
void
ieee_SUBT (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	a_type = extend_ieee(a, &op_a, DOUBLE);
	b_type = extend_ieee(b, &op_b, DOUBLE);

	if( (POSINFX(a)&&POSINFX(b)) || (NEGINFX(a)&&NEGINFX(b)) ){
	  SET_FPSCR_VXISI_VAL(*pi);SET_FPSCR_FX_VAL(*pi);
	}

	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}



	if ((a_type >= NaN && a_type <= INFTY) ||
	    (b_type >= NaN && b_type <= INFTY))
	{
		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | ( ((unsigned long long)0x1)<< 51);

		
		if (a_type == NaN || b_type == NaN)
			return;
		if (a_type == QNaN || b_type == QNaN)
			return;


		if (a_type == INFTY && b_type == INFTY && sign(a) == sign(b)) {
			*c = IEEE_QNaN;
			return;
		}
		if (a_type == INFTY)
			*c = a;
		else
			*c = b ^ (((unsigned long long)0x1) << 63);
		return;
	}
	op_b.s = !op_b.s;
	add_kernel_ieee(&op_a, &op_b, &op_c);
	/* special case for -0 - +0 ==> -0 */
	if (a_type == ZERO && b_type == ZERO)
		op_c.s = op_a.s && op_b.s;

	round_t_ieee(f, &op_c, c,pi);
}



void
ieee_SUBS (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	a_type = extend_ieee(a, &op_a, SINGLE);
	b_type = extend_ieee(b, &op_b, SINGLE);
	
	if( (POSINFSPX(a)&&POSINFSPX(b))||(NEGINFSPX(a)&&NEGINFSPX(b))){
	  SET_FPSCR_VXISI_VAL(*pi); SET_FPSCR_FX_VAL(*pi);
	}

	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}

	if ((a_type >= NaN && a_type <= INFTY) ||
	    (b_type >= NaN && b_type <= INFTY))
	{
		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | ( ((unsigned long long)0x1)<< 51);

		if (a_type == NaN || b_type == NaN)
			return;
		if (a_type == QNaN || b_type == QNaN)
			return;

		if (a_type == INFTY && b_type == INFTY && sign(a) == sign(b)) {
			*c = IEEE_QNaN;
			return;
		}
		if (a_type == INFTY)
			*c = a;
		else
			*c = b ^ (((unsigned long long)0x1) << 63);
		return;
	}
	op_b.s = !op_b.s;
	add_kernel_ieee(&op_a, &op_b, &op_c);
	/* special case for -0 - +0 ==> -0 */
	if (a_type == ZERO && b_type == ZERO)
		op_c.s = op_a.s && op_b.s;

       round_s_ieee(f, &op_c, c,pi);
}


/*
 * Multiply a x b = c, where a, b, and c are ieee t-floating numbers.
 * "f" contains the rounding mode.
 */
void
ieee_MULT (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	*c = IEEE_QNaN;
	a_type = extend_ieee(a, &op_a, DOUBLE);
	b_type = extend_ieee(b, &op_b, DOUBLE);

	if( (a_type==ZERO) &&( POSINFX(b)||(NEGINFX(b)) ) ){
	  SET_FPSCR_VXIMZ_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}
	if((b_type==ZERO)&&(POSINFX(a)||NEGINFX(a))){
	  SET_FPSCR_VXIMZ_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}
	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}



	
	if ((a_type >= NaN && a_type <= ZERO) ||
	    (b_type >= NaN && b_type <= ZERO))
	{
		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | ( ((unsigned long long)0x1) << 51);

		if (a_type == NaN || b_type == NaN)
			return ;
		if (a_type == QNaN || b_type == QNaN)
			return ;

		if ((a_type == INFTY && b_type == ZERO) ||
		    (b_type == INFTY && a_type == ZERO))
		{
			*c = IEEE_QNaN;		/* return canonical QNaN */
			return ;
		}
		if (a_type == INFTY)
			*c = a ^ ((b >> 63) << 63);
		else if (b_type == INFTY)
			*c = b ^ ((a >> 63) << 63);
		else
			/* either of a and b are +/-0 */
			*c = ((unsigned long long) op_a.s ^ op_b.s) << 63;
		return ;
	}
	op_c.s = op_a.s ^ op_b.s;
	op_c.e = op_a.e + op_b.e;
	
       	mul64(op_a.f[0], op_b.f[0], op_c.f);

	normalize(&op_c);
	op_c.e -= 55;	/* drop the 55 original bits. */

        round_t_ieee(f, &op_c, c, pi);
}


void
ieee_MULS (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	*c = IEEE_QNaN;
	a_type = extend_ieee(a, &op_a, SINGLE);
	b_type = extend_ieee(b, &op_b, SINGLE);

	if((a_type==ZERO)&&(POSINFX(b)||NEGINFX(b))){
	  SET_FPSCR_VXIMZ_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}
	if((b_type==ZERO)&&(POSINFX(a)||NEGINFX(a))){
	  SET_FPSCR_VXIMZ_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}
	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}



	if ((a_type >= NaN && a_type <= ZERO) ||
	    (b_type >= NaN && b_type <= ZERO))
	{
		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | (((unsigned long long)0x1) << 51);

		if (a_type == NaN || b_type == NaN)
			return ;
		if (a_type == QNaN || b_type == QNaN)
			return ;

		if ((a_type == INFTY && b_type == ZERO) ||
		    (b_type == INFTY && a_type == ZERO))
		{
			*c = IEEE_QNaN;		/* return canonical QNaN */
			return ;
		}
		if (a_type == INFTY)
			*c = a ^ ((b >> 63) << 63);
		else if (b_type == INFTY)
			*c = b ^ ((a >> 63) << 63);
		else
			/* either of a and b are +/-0 */
			*c = ((unsigned long long) op_a.s ^ op_b.s) << 63;
		return ;
	}
	op_c.s = op_a.s ^ op_b.s;
	op_c.e = op_a.e + op_b.e;
	
	mul64(op_a.f[0], op_b.f[0], op_c.f);


	normalize(&op_c);
	op_c.e -= 55;	/* drop the 55 original bits. */

        round_t_ieee(f, &op_c, c, pi);
}

void
ieee_DIVS (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	*c = IEEE_QNaN;
	a_type = extend_ieee(a, &op_a, SINGLE);
	b_type = extend_ieee(b, &op_b, SINGLE);

	if((POSINFX(a)||NEGINFX(a))&&(POSINFX(b)||NEGINFX(b)))
	  {
	    SET_FPSCR_VXIDI_VAL(*pi); SET_FPSCR_FX_VAL(*pi);
	  }
	if((a_type==ZERO)&&(b_type==ZERO))
	  {
	    SET_FPSCR_VXIDI_VAL(*pi); SET_FPSCR_FX_VAL(*pi);
	  }
	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}

	if ((a_type >= NaN && a_type <= ZERO) ||
	    (b_type >= NaN && b_type <= ZERO))
	{


		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | (((unsigned long long)0x1) << 51);

		if (a_type == NaN || b_type == NaN)
			return ;
		if (a_type == QNaN || b_type == QNaN)
			return ;

		
		*c = IEEE_PINF;
		if (a_type == INFTY) {
			if (b_type == INFTY) {
				*c = IEEE_QNaN;
				return;
			}
		} else if (b_type == ZERO) {
			if (a_type == ZERO) {
				*c = IEEE_QNaN;
				return;
			}

		} else
			/* a_type == ZERO || b_type == INFTY */
			*c = 0;
		*c |= (unsigned long long) (op_a.s ^ op_b.s) << 63;
		return;
	}
	op_c.s = op_a.s ^ op_b.s;
	op_c.e = op_a.e - op_b.e;
	
	op_a.f[1] = op_a.f[0];
	op_a.f[0] = 0;
	div128(op_a.f, op_b.f, op_c.f);
	if (a_type != ZERO)
		/* force a sticky bit because DIVs never hit exact .5: */
		op_c.f[0] |= STICKY_S;
	normalize(&op_c);
	op_c.e -= 9;		/* remove excess exp from original shift */
        round_t_ieee(f, &op_c, c, pi);
}



void
ieee_DIVT (int f, unsigned long long a, unsigned long long b, unsigned long long *c, unsigned int *pi)
{
	fpclass_t a_type, b_type;
	EXTENDED op_a, op_b, op_c;

	*c = IEEE_QNaN;
	a_type = extend_ieee(a, &op_a, SINGLE);
	b_type = extend_ieee(b, &op_b, SINGLE);

	b_type = extend_ieee(b, &op_b, DOUBLE);
	if((POSINFX(a)||NEGINFX(a))&&(POSINFX(b)||NEGINFX(b)))
	  {
	    SET_FPSCR_VXIDI_VAL(*pi); SET_FPSCR_FX_VAL(*pi);
	  }
	if((a_type==ZERO)&&(b_type==ZERO))
	  {
	    SET_FPSCR_VXIDI_VAL(*pi); SET_FPSCR_FX_VAL(*pi);
	  }
	if( a_type==NaN||b_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	}


	if ((a_type >= NaN && a_type <= ZERO) ||
	    (b_type >= NaN && b_type <= ZERO))
	{


		/* propagate NaNs according to arch. ref. handbook: */
		if (b_type == QNaN)
			*c = b;
		else if (b_type == NaN)
			*c = b | (((unsigned long long)0x1) << 51);
		else if (a_type == QNaN)
			*c = a;
		else if (a_type == NaN)
			*c = a | (((unsigned long long)0x1) << 51);

		if (a_type == NaN || b_type == NaN)
			return ;
		if (a_type == QNaN || b_type == QNaN)
			return ;


		*c = IEEE_PINF;
		if (a_type == INFTY) {
			if (b_type == INFTY) {
				*c = IEEE_QNaN;
				return;
			}
		} else if (b_type == ZERO) {
			if (a_type == ZERO) {
				*c = IEEE_QNaN;
				return;
			}

		} else
			/* a_type == ZERO || b_type == INFTY */
			*c = 0;
		*c |= (unsigned long long) (op_a.s ^ op_b.s) << 63;
		return;
	}
	op_c.s = op_a.s ^ op_b.s;
	op_c.e = op_a.e - op_b.e;


	op_a.f[1] = op_a.f[0];
	op_a.f[0] = 0;
	div128(op_a.f, op_b.f, op_c.f);

	if (a_type != ZERO)
		/* force a sticky bit because DIVs never hit exact .5 */
		op_c.f[0] |= STICKY_T;
	normalize(&op_c);
	op_c.e -= 9;		/* remove excess exp from original shift */
        round_t_ieee(f, &op_c, c, pi);
}




void printull(unsigned long long u){

unsigned int *pi = (unsigned int *)&u;
printf("%x",*pi);
pi++;
printf("%x\n",*pi);

}



void
ieee_CVTW (int f, unsigned long long a, unsigned long long *b, unsigned int *pi)
{
	unsigned int midway;
	unsigned long long ov, uv = 0;
	fpclass_t a_type;
	EXTENDED temp;

	*b = 0;
	a_type = extend_ieee(a, &temp, DOUBLE);

	if (a_type == INFTY){
	  RESET_FPSCR_FR_VAL(*pi);
	  RESET_FPSCR_FI_VAL(*pi);
	  SET_FPSCR_VXCVI_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	  if(a&0x8000000000000000ll){
	    /*-ve*/
	    *b=0x7fffffff;
	    
	  }
	  else{
	    *b=0x80000000;
	  }
	  return;
	}

	if (a_type == QNaN){
	  RESET_FPSCR_FR_VAL(*pi);
	  RESET_FPSCR_FI_VAL(*pi);
	  SET_FPSCR_VXCVI_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	  *b=0x80000000;
	  return;
	}


	if (a_type == NaN){
	  RESET_FPSCR_FR_VAL(*pi);
	  RESET_FPSCR_FI_VAL(*pi);
	  SET_FPSCR_VXCVI_VAL(*pi);
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	  *b=0x80000000;
	  return;
	}

	if((temp.e+0x3ff)>1054){
	  RESET_FPSCR_FR_VAL(*pi);
	  RESET_FPSCR_FI_VAL(*pi);
	  SET_FPSCR_VXCVI_VAL(*pi);
	  SET_FPSCR_FX_VAL(*pi);
	   if(a&0x8000000000000000ll){
	    /*-ve*/
	    *b=0x80000000;
	    
	  }
	  else{
	    *b=0x7fffffff;
	  }
	}

	if (temp.e > 0) {
		ov = 0;
		while (temp.e > 0) {
			--temp.e;
			ov |= temp.f[1] >> 63;
			sll128(temp.f);
		}
		if (ov || (temp.f[1] & 0xffc0000000000000ll)){
		  SET_FPSCR_FI_VAL(*pi);
		  SET_FPSCR_XX_VAL(*pi);
		}
	}
	if (temp.e < 0) {
		while (temp.e < 0) {
			++temp.e;
			uv = temp.f[0] & 1;		/* save sticky bit */
			srl128(temp.f);
			temp.f[0] |= uv;
		}
	}
	*b = ((temp.f[1] << 9) | (temp.f[0] >> 55)) & 0x7fffffffffffffffll;
	/*
	 * Notice: the fraction is only 52 bits long.  Thus, rounding
	 * cannot possibly result in an integer overflow.
	 */
	switch (f) {
	      case ROUND_NEAR:
		if (temp.f[0] & 0x0040000000000000ll) {
			midway = (temp.f[0] & 0x003fffffffffffffll) == 0;
			if ((midway && (temp.f[0] & 0x0080000000000000ll)) ||
			    !midway){
				++(*b);
				SET_FPSCR_FR_VAL(*pi);
			}
		}
		break;

	      case ROUND_PINF:
		if ((temp.f[0] & 0x003fffffffffffffll) != 0){
			++(*b);
			SET_FPSCR_FR_VAL(*pi);
		}
		break;

	      case ROUND_NINF:
		if ((temp.f[0] & 0x003fffffffffffffll) != 0){
			--(*b);
			SET_FPSCR_FR_VAL(*pi);	
		}
		break;

	      case ROUND_CHOP:
		/* no action needed */
		break;
	}
	if ((temp.f[0] & 0x003fffffffffffffll) != 0)
		{
		 SET_FPSCR_FI_VAL(*pi);
		 SET_FPSCR_XX_VAL(*pi);
		}


	if (temp.s) {
	  *b = -*b;
	  if((long long)(*b)<(long long)(int)(0x80000000)){
		  RESET_FPSCR_FR_VAL(*pi);
		  RESET_FPSCR_FI_VAL(*pi);
		  SET_FPSCR_VXCVI_VAL(*pi);
		  SET_FPSCR_FX_VAL(*pi);
		  *b=0x80000000;		    
	  }
	}
	else{
	  if(*b>0x7fffffff){
	    /*Large op*/
	    RESET_FPSCR_FR_VAL(*pi);
	    RESET_FPSCR_FI_VAL(*pi);
	    SET_FPSCR_VXCVI_VAL(*pi);
	    SET_FPSCR_FX_VAL(*pi);
	    *b=0x7fffffff;
	  }
	}
	return;
}

void
ieee_CVTTS (int f, unsigned long long a, unsigned long long *b, unsigned int *pi)
{
	EXTENDED temp;
	fpclass_t a_type;
	unsigned int expval;

	a_type = extend_ieee(a, &temp, DOUBLE);
	expval = temp.e +0x3ff;
	if(a_type==INFTY){
	  *b = a;
	  if(temp.s) SET_FPSCR_FPRF_VAL(*pi, 0x9);
	  else  SET_FPSCR_FPRF_VAL(*pi, 0x3);

	  return;
	}
	else if(a_type==ZERO){
	  *b = a;
	  RESET_FPSCR_FI_VAL(*pi);
	  RESET_FPSCR_FR_VAL(*pi);
	  if(temp.s) SET_FPSCR_FPRF_VAL(*pi, 0x12);
	  else  SET_FPSCR_FPRF_VAL(*pi, 0x2);
	  return;
	}
	else if(a_type==QNaN){
	   
	  *b= a&0xffffffffE0000000ll;
	  RESET_FPSCR_FI_VAL(*pi);
	  RESET_FPSCR_FR_VAL(*pi);
	  SET_FPSCR_FPRF_VAL(*pi,0x11);
	  return;
	}
	else if(a_type==NaN){
	  SET_FPSCR_VXSNAN_VAL(*pi);
	  
	  if(GET_FPSCR_VE_VAL(*pi)){
	  
	    *b=a;
	    *b |= (((unsigned long long)0x1) << 51);
	    *b = *b & 0xffffffffE0000000ll;
	    SET_FPSCR_VXSNAN_VAL(*pi);
	    SET_FPSCR_FX_VAL(*pi);
	    RESET_FPSCR_FI_VAL(*pi);
	    RESET_FPSCR_FR_VAL(*pi);
	    SET_FPSCR_FPRF_VAL(*pi,0x11);
	  }
	  return;
	}
	
	
	round_s_ieee(f, &temp, b, pi);
	setFPSCRFPRFFlags(*b,pi);

}

void negate(unsigned long long a, unsigned long long *b, unsigned int *pi){
  a=a ^ 0x8000000000000000ll;
  setFPSCRFPRFFlags( *b, pi);
}
  

void setFPSCRFPRFFlags(unsigned long long b, unsigned int *pi)
{
  if(b&0x8000000000000000ll){
    /*Negative*/
    if((b&0x7fffffffffffffffll)==0){
      /*-Zero*/
      SET_FPSCR_FPRF_VAL(*pi,0x12);
      return;
    }
    if(((b&0x7ff0000000000000ll)==0)&&((b&0x000fffffffffffffll)!=0)){
      /*-DENORM*/
      SET_FPSCR_FPRF_VAL(*pi,0x18);
      return;
    }
    if(((b&0x7ff0000000000000ll)==0x7ff0000000000000ll)&&((b&0x000fffffffffffffll)==0)){
      /*-INF*/
      SET_FPSCR_FPRF_VAL(*pi,0x9);
      return;
    }
    
    if(((b&0x7ff0000000000000ll)==0x7ff0000000000000ll)&&((b&0x0008000000000000ll)!=0)){
      /*NaN*/
      SET_FPSCR_FPRF_VAL(*pi,0x11);
      return;
    } 

    SET_FPSCR_FPRF_VAL(*pi,0x8);
    return;
  }
  else{
    
    if((b&0x7fffffffffffffffll)==0){
      /*+Zero*/
      SET_FPSCR_FPRF_VAL(*pi,0x2);
      return;
      }
    
    if(((b&0x7ff0000000000000ll)==0)&&((b&0x000fffffffffffffll)!=0)){
      /*+DENORM*/
      SET_FPSCR_FPRF_VAL(*pi,0x14);
      return;
    }
    if(((b&0x7ff0000000000000ll)==0x7ff0000000000000ll)&&((b&0x000fffffffffffffll)==0)){
      /*+INF*/
      SET_FPSCR_FPRF_VAL(*pi,0x3);
      return;
    }
    if(((b&0x7ff0000000000000ll)==0x7ff0000000000000ll)&&((b&0x0008000000000000ll)!=0)){
      /*NaN*/
      SET_FPSCR_FPRF_VAL(*pi,0x11);
      return;
    }
    SET_FPSCR_FPRF_VAL(*pi,0x4);
    return;
  }

  
}


/*

int main (){
double _da, _db;
unsigned long long a,b,c;
unsigned int i=0;
_da = 0.0;
_db=1.5;
a = *(unsigned long long *)&_da;
b = *(unsigned long long *)&_db;

ieee_MULT(0,a,b,&c,&i);
printull(b);
printull(c);

printf("%f\n", *(double *)&c);
return 0;
}





*/
