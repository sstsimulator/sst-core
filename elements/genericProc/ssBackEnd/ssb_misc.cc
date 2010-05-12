/*
 * misc.c - miscellaneous routines
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: ssb_misc.cc,v 1.3 2007-09-11 23:49:47 wcmclen Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2007/06/05 20:42:14  ktpedre
 * Linux/Glibc has random() but it appears it can't be called from C++ apps.
 * Add a special define, _USE_RAND to allow Linux builds to use rand()
 * instead of random().
 *
 * Revision 1.1.1.1  2006/01/31 16:35:50  afrodri
 * first entry
 *
 * Revision 1.4  2005/08/29 21:40:08  arodrig6
 * update for 10.4
 *
 * Revision 1.3  2005/06/28 21:21:22  arodrig6
 * changes for GCC 4.0 - mainly making simpleScalar tighter
 *
 * Revision 1.2  2004/08/10 22:55:35  arodrig6
 * works, except for the speculative stuff, caches, and multiple procs
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.3  2000/04/11 00:55:56  karu
 * made source platform independent. compiles on linux, solaris and ofcourse aix.
 * powerpc-nonnative.def is the def file with no inline assemble code.
 * manually create the machine.def symlink if compiling on non-aix OS.
 * also syscall.c does not compile because the syscalls haven't been
 * resourced with diff. flags and names for different operating systems.
 *
 * Revision 1.2  2000/04/10 23:46:31  karu
 * converted all quad to qword. NO OTHER change made.
 * the tag specfp95-before-quad-qword is a snapshow before this change was made
 *
 * Revision 1.1.1.1  2000/03/07 05:15:17  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:50  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.6  1998/08/27 15:41:27  taustin
 * implemented host interface description in host.h
 * added target interface support
 * implemented a more portable random() interface
 * disabled calls to sbrk() under malloc(), this breaks some
 *       malloc() implementation (e.g., newer Linux releases)
 * added myprintf() and myatoq() routines for printing and reading
 *       quadword's, respectively
 * added gzopen() and gzclose() routines for reading and writing
 *       compressed files, updated sysprobe to search for GZIP, if found
 *       support is enabled
 *
 * Revision 1.5  1997/03/11  01:17:36  taustin
 * updated copyright
 * supported added for non-GNU C compilers
 *
 * Revision 1.4  1997/01/06  16:01:45  taustin
 * comments updated
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#if defined(__APPLE__)  // ugly hack. for some reason, __error confuses it
#define KERNEL
#endif
#include <errno.h>
#if defined(__APPLE__)
#undef KERNEL
#endif

#if defined(__alpha) || defined(linux)
#include <unistd.h>
#endif /* __alpha || linux */

#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_machine.h"

/* verbose output flag */
int verbose = FALSE;

#ifdef DEBUG
/* active debug flag */
int debugging = FALSE;
#endif /* DEBUG */

/* fatal function hook, this function is called just before an exit
   caused by a fatal error, used to spew stats, etc. */
static void (*hook_fn)(FILE *stream) = NULL;

/* register a function to be called when an error is detected */
void
fatal_hook(void (*fn)(FILE *stream))	/* fatal hook function */
{
  hook_fn = fn;
}

/* declare a fatal run-time error, calls fatal hook function */
#ifdef __GNUC__
void
_fatal(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
fatal(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "fatal: ");
  myvfprintf(stderr, (char*)fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  if (hook_fn)
    (*hook_fn)(stderr);
  exit(1);
}

/* declare a panic situation, dumps core */
#ifdef __GNUC__
void
_panic(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
panic(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "panic: ");
  myvfprintf(stderr, (char*)fmt, v);
#ifdef __GNUC__
  fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  if (hook_fn)
    (*hook_fn)(stderr);
  abort();
}

/* declare a warning */
#ifdef __GNUC__
void
_warn(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
warn(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "warning: ");
  myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
}

/* print general information */
#ifdef __GNUC__
void
_info(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
info(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
}

#ifdef DEBUG
/* print a debugging message */
#ifdef __GNUC__
void
_debug(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
debug(char *fmt, ...)
#endif /* __GNUC__ */
{
    va_list v;
    va_start(v, fmt);

    if (debugging)
      {
        fprintf(stderr, "debug: ");
        myvfprintf(stderr, fmt, v);
#ifdef __GNUC__
        fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif
        fprintf(stderr, "\n");
      }
}
#endif /* DEBUG */


/* seed the random number generator */
void
mysrand(unsigned int seed)	/* random number generator seed */
{
#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) || defined(__svr4__) || defined(_MSC_VER) || defined(_USE_RAND)
      srand(seed);
#else
      srandom(seed);
#endif
}

/* get a random number */
int
myrand(void)			/* returns random number */
{
#if !defined(__alpha) && !defined(__APPLE__)
#ifdef linux
    // wcm: this was random() but x86 linux linker had a problem???
    //      rand() seems to be ok for now... need to research what
    //      was upsetting the linker so much.
  extern uint32_t rand(void);
#else
  extern long random(void);
#endif
#endif

#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) || defined(__svr4__) || defined(_MSC_VER) || defined(_USE_RAND)
  return rand();
#else
  return random();
#endif
}

/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(const char *s)		/* string to duplicate to heap storage */
{
  char *buf;

  if (!(buf = (char *)malloc(strlen(s)+1)))
    return NULL;
  strcpy(buf, s);
  return buf;
}

/* find the last occurrence of a character in a string */
char *
mystrrchr(char *s, char c)
{
  char *rtnval = 0;

  do {
    if (*s == c)
      rtnval = s;
  } while (*s++);

  return rtnval;
}

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(const char *s1, const char *s2)	/* strings to compare, case insensitive */
{
  unsigned char u1, u2;

  for (;;)
    {
      u1 = (unsigned char)*s1++; u1 = tolower(u1);
      u2 = (unsigned char)*s2++; u2 = tolower(u2);

      if (u1 != u2)
	return u1 - u2;
      if (u1 == '\0')
	return 0;
    }
}

/* allocate some core, this memory has overhead no larger than a page
   in size and it cannot be released. the storage is returned cleared */
void *
getcore(int nbytes)
{
  return calloc(nbytes, 1);

#if 0 /* FIXME: sbrk() calls break malloc() on Linux... */
#if !defined(PURIFY) && !defined(_MSC_VER)
  void *p = (void *)sbrk(nbytes);

  if (p == (void *)-1)
    return NULL;

  /* this may be superfluous */
#if defined(__svr4__) || defined(_MSC_VER)
  memset(p, '\0', nbytes);
#else /* !defined(__svr4__) */
  bzero(p, nbytes);
#endif
  return p;
#else
  return calloc(nbytes, 1);
#endif /* PURIFY */
#endif
}

/* return log of a number to the base 2 */
int
log_base2(int n)
{
  int power = 0;

  if (n <= 0 || (n & (n-1)) != 0)
    panic("log2() only works for positive power of two values");

  while (n >>= 1)
    power++;

  return power;
}

/* return string describing elapsed time, passed in SEC in seconds */
const char *
elapsed_time(long sec)
{
  static char tstr[256];
  char temp[256];

  if (sec <= 0)
    return "0s";

  tstr[0] = '\0';

  /* days */
  if (sec >= 86400)
    {
      sprintf(temp, "%ldD ", sec/86400);
      strcat(tstr, temp);
      sec = sec % 86400;
    }
  /* hours */
  if (sec >= 3600)
    {
      sprintf(temp, "%ldh ", sec/3600);
      strcat(tstr, temp);
      sec = sec % 3600;
    }
  /* mins */
  if (sec >= 60)
    {
      sprintf(temp, "%ldm ", sec/60);
      strcat(tstr, temp);
      sec = sec % 60;
    }
  /* secs */
  if (sec >= 1)
    {
      sprintf(temp, "%lds ", sec);
      strcat(tstr, temp);
    }
  tstr[strlen(tstr)-1] = '\0';
  return tstr;
}

/* assume bit positions numbered 31 to 0 (31 high order bit), extract num bits
   from word starting at position pos (with pos as the high order bit of those
   to be extracted), result is right justified and zero filled to high order
   bit, for example, extractl(word, 6, 3) w/ 8 bit word = 01101011 returns
   00000110 */
unsigned int
extractl(int word,		/* the word from which to extract */
         int pos,		/* bit positions 31 to 0 */
         int num)		/* number of bits to extract */
{
    return(((unsigned int) word >> (pos + 1 - num)) & ~(~0 << num));
}

#define PUT(p, n)							\
  {									\
    int nn, cc;								\
									\
    for (nn = 0; nn < n; nn++)						\
      {									\
	cc = *(p+nn);							\
        *obuf++ = cc;							\
      }									\
  }

#define PAD(s, n)							\
  {									\
    int nn, cc;								\
									\
    cc = *s;								\
    for (nn = n; nn > 0; nn--)						\
      *obuf++ = cc;							\
  }

#ifdef HOST_HAS_QWORD
#define HIBITL		LL(0x8000000000000000)
typedef sqword_t slargeint_t;
typedef qword_t largeint_t;
#else /* !HOST_HAS_QWORD */
#define HIBITL		0x80000000L
typedef sword_t slargeint_t;
typedef word_t largeint_t;
#endif /* HOST_HAS_QWORD */

static int
_lowdigit(slargeint_t *valptr)
{
  /* this function computes the decimal low-order digit of the number pointed
     to by valptr, and returns this digit after dividing *valptr by ten; this
     function is called ONLY to compute the low-order digit of a long whose
     high-order bit is set */

  int lowbit = (int)(*valptr & 1);
  slargeint_t value = (*valptr >> 1) & ~HIBITL;

  *valptr = value / 5;
  return (int)(value % 5 * 2 + lowbit + '0');
}

/* portable vsprintf with quadword support, returns end pointer */
char *
myvsprintf(char *obuf, const char *format, va_list v)
{
  static char _blanks[] = "                    ";
  static char _zeroes[] = "00000000000000000000";

  /* counts output characters */
  int count = 0;

  /* format code */
  int fcode;

  /* field width and precision */
  int width, prec=0;

  /* number of padding zeroes required on the left and right */
  int lzero=0;

  /* length of prefix */
  int prefixlength;

  /* combined length of leading zeroes, trailing zeroes, and suffix */
  int otherlength;

  /* format flags */
#define PADZERO		0x0001	/* padding zeroes requested via '0' */
#define RZERO		0x0002	/* there will be trailing zeros in output */
#define LZERO		0x0004	/* there will be leading zeroes in output */
#define DOTSEEN		0x0008	/* dot appeared in format specification */
#define LENGTH		0x0010	/* l */
  int flagword;

  /* maximum number of digits in printable number */
#define MAXDIGS		22

  /* starting and ending points for value to be printed */
  char *bp, *p;

  /* work variables */
  int k, lradix, mradix;

  /* pointer to sign, "0x", "0X", or empty */
  const char *prefix=0;

  /* values are developed in this buffer */
  static char buf[MAXDIGS*4], buf1[MAXDIGS*4];

  /* pointer to a translate table for digits of whatever radix */
  char *tab;

  /* value being converted, if integer */
  slargeint_t val;

  /* value being converted, if floating point */
  dfloat_t fval;

  for (;;)
    {
      int n;

      while ((fcode = *format) != '\0' && fcode != '%')
	{
	  *obuf++ = fcode;
	  format++;
	  count++;
	}

      if (fcode == '\0')
	{
	  /* end of format; terminate and return */
	  *obuf = '\0';
	  return obuf;
	}


      /* % has been found, the following switch is used to parse the format
	 specification and to perform the operation specified by the format
	 letter; the program repeatedly goes back to this switch until the
	 format letter is encountered */

      width = prefixlength = otherlength = flagword = 0;
      format++;

    charswitch:
      switch (fcode = *format++)
	{
	case '0': /* means pad with leading zeros */
	  flagword |= PADZERO;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  {
	    int num = fcode - '0';
	    while (isdigit(fcode = *format))
	      {
		num = num * 10 + fcode - '0';
		format++;
	      }
	    if (flagword & DOTSEEN)
	      prec = num;
	    else
	      width = num;
	    goto charswitch;
	  }

	case '.':
	  flagword |= DOTSEEN;
	  goto charswitch;

	case 'l':
	  flagword |= LENGTH;
	  goto charswitch;

	case 'n': /* host counter */
#ifdef HOST_HAS_QWORD
	  flagword |= LENGTH;
	  /* fallthru */
#else /* !HOST_HAS_QWORD */
	  flagword |= DOTSEEN;
	  if (!width)
	    width = 12;
	  prec = 0;
	  goto process_float;
#endif /* HOST_HAS_QWORD */
	  
	case 'd':
	  /* fetch the argument to be printed */
	  if (flagword & LENGTH)
	    val = va_arg(v, slargeint_t);
	  else
	    val = (slargeint_t)va_arg(v, sword_t);

	  /* set buffer pointer to last digit */
	  p = bp = buf + MAXDIGS;

	  /* If signed conversion, make sign */
	  if (val < 0)
	    {
	      prefix = "-";
	      prefixlength = 1;
	      /* negate, checking in advance for possible overflow */
	      if (val != (slargeint_t)HIBITL)
		val = -val;
	      else
		{
		  /* number is -HIBITL; convert last digit and get pos num */
		  *--bp = _lowdigit(&val);
		}
	    }

	decimal:
	  {
	    slargeint_t qval = val;

	    if (qval <= 9)
	      *--bp = (int)qval + '0';
	    else
	      {
		do {
		  n = (int)qval;
		  qval /= 10;
		  *--bp = n - (int)qval * 10 + '0';
		}
		while (qval > 9);
		*--bp = (int)qval + '0';
	      }
	  }
	  break;

	case 'u':
	  /* fetch the argument to be printed */
	  if (flagword & LENGTH)
	    val = va_arg(v, largeint_t);
	  else
	    val = (largeint_t)va_arg(v, word_t);

	  /* set buffer pointer to last digit */
	  p = bp = buf + MAXDIGS;

	  if (val & HIBITL)
	    *--bp = _lowdigit(&val);
	  goto decimal;

	case 'o':
	  mradix = 7;
	  lradix = 2;
	  goto fixed;

	case 'p': /* target address */
	  if (sizeof(md_addr_t) > 4)
	    flagword |= LENGTH;
	  /* fallthru */

	case 'X':
	case 'x':
	  mradix = 15;
	  lradix = 3;

	fixed:
	  /* fetch the argument to be printed */
	  if (flagword & LENGTH)
	    val = va_arg(v, largeint_t);
	  else
	    val = (largeint_t)va_arg(v, word_t);

	  /* set translate table for digits */
	  tab = (char*)((fcode == 'X') ? "0123456789ABCDEF" : "0123456789abcdef");

	  /* develop the digits of the value */
	  p = bp = buf + MAXDIGS;

	  {
	    slargeint_t qval = val;

	    if (qval == 0)
	      {
		otherlength = lzero = 1;
		flagword |= LZERO;
	      }
	    else
	      do {
		*--bp = tab[qval & mradix];
		qval = ((qval >> 1) & ~HIBITL) >> lradix;
	      } while (qval != 0);
	  }
	  break;

#ifndef HOST_HAS_QWORD
	process_float:
#endif /* !HOST_HAS_QWORD */

	case 'f':
	  if (flagword & DOTSEEN)
	    sprintf(buf1, "%%%d.%df", width, prec);
	  else if (width)
	    sprintf(buf1, "%%%df", width);
	  else
	    sprintf(buf1, "%%f");

	  /* fetch the argument to be printed */
	  fval = va_arg(v, dfloat_t);

	  /* print floating point value */
	  sprintf(buf, buf1, fval);
	  bp = buf;
	  p = bp + strlen(bp);
	  break;

	case 's':
	  bp = va_arg(v, char *);
	  if (bp == NULL)
	    bp = (char*) "(null)";
	  p = bp + strlen(bp);
	  break;

	case '%':
	  buf[0] = fcode;
	  goto c_merge;

	case 'c':
	  buf[0] = va_arg(v, int);
	c_merge:
	  p = (bp = &buf[0]) + 1;
	  break;

	default:
	  /* this is technically an error; what we do is to back up the format
             pointer to the offending char and continue with the format scan */
	  format--;
	  continue;
	}

      /* calculate number of padding blanks */
      k = (n = p - bp) + prefixlength + otherlength;
      if (width <= k)
	count += k;
      else
	{
	  count += width;

	  /* set up for padding zeroes if requested; otherwise emit padding
             blanks unless output is to be left-justified */
	  if (flagword & PADZERO)
	    {
	      if (!(flagword & LZERO))
		{
		  flagword |= LZERO;
		  lzero = width - k;
		}
	      else
		lzero += width - k;

	      /* cancel padding blanks */
	      k = width;
	    }
	  else
	    {
	      /* blanks on left if required */
	      PAD(_blanks, width - k);
	    }
	}

      /* prefix, if any */
      if (prefixlength != 0)
	{
	  PUT(prefix, prefixlength);
	}

      /* zeroes on the left */
      if (flagword & LZERO)
	{
	  PAD(_zeroes, lzero);
	}

      /* the value itself */
      if (n > 0)
	{
	  PUT(bp, n);
	}
    }
}

/* portable sprintf with quadword support, returns end pointer */
char *
mysprintf(char *obuf, const char *format, ...)
{
  /* vararg parameters */
  va_list v;
  va_start(v, format);

  return myvsprintf(obuf, format, v);
}

/* portable vfprintf with quadword support, returns end pointer */
void
myvfprintf(FILE *stream, const char *format, va_list v)
{
  /* temp buffer */
  char buf[512];

  myvsprintf(buf, format, v);
  fputs(buf, stream);
}

/* portable fprintf with quadword support, returns end pointer */
void
myfprintf(FILE *stream, const char *format, ...)
{
  /* temp buffer */
  char buf[512];

  /* vararg parameters */
  va_list v;
  va_start(v, format);

  myvsprintf(buf, format, v);
  fputs(buf, stream);
}

#ifdef HOST_HAS_QWORD

#define LL_MAX		LL(9223372036854775807)
#define LL_MIN		(-LL_MAX - 1)
#define ULL_MAX		(ULL(9223372036854775807) * ULL(2) + 1)

/* convert a string to a signed result */
sqword_t
myatosq(char *nptr, char **endp, int base)
{
  char *s, *save;
  int negative, overflow;
  sqword_t cutoff, cutlim, i;
  unsigned char c;
#ifndef errno
  extern int errno;
#endif

  if (!nptr || !*nptr)
    panic("strtoll() passed a NULL string");

  s = nptr;

  /* skip white space */
  while (isspace((int)(*s)))
    ++s;
  if (*s == '\0')
    goto noconv;

  if (base == 0)
    {
      if (s[0] == '0' && toupper(s[1]) == 'X')
	base = 16;
      else
	base = 10;
    }

  if (base <= 1 || base > 36)
    panic("bogus base: %d", base);

  /* check for a sign */
  if (*s == '-')
    {
      negative = 1;
      ++s;
    }
  else if (*s == '+')
    {
      negative = 0;
      ++s;
    }
  else
    negative = 0;

  if (base == 16 && s[0] == '0' && toupper(s[1]) == 'X')
    s += 2;

  /* save the pointer so we can check later if anything happened */
  save = s;

  cutoff = LL_MAX / (unsigned long int) base;
  cutlim = LL_MAX % (unsigned long int) base;

  overflow = 0;
  i = 0;
  for (c = *s; c != '\0'; c = *++s)
    {
      if (isdigit (c))
        c -= '0';
      else if (isalpha (c))
        c = toupper(c) - 'A' + 10;
      else
        break;
      if (c >= base)
        break;

      /* check for overflow */
      if (i > cutoff || (i == cutoff && c > cutlim))
        overflow = 1;
      else
        {
          i *= (unsigned long int) base;
          i += c;
        }
    }

  /* check if anything actually happened */
  if (s == save)
    goto noconv;

  /* store in ENDP the address of one character past the last character
     we converted */
  if (endp != NULL)
    *endp = (char *) s;

  if (overflow)
    {
      errno = ERANGE;
      return negative ? LL_MIN : LL_MAX;
    }
  else
    {
      errno = 0;

      /* return the result of the appropriate sign */
      return (negative ? -i : i);
    }

noconv:
  /* there was no number to convert */
  if (endp != NULL)
    *endp = (char *) nptr;
  return 0;
}

/* convert a string to a unsigned result */
qword_t
myatoq(char *nptr, char **endp, int base)
{
  char *s, *save;
  int overflow;
  qword_t cutoff, cutlim, i;
  unsigned char c;
#ifndef errno
  extern int errno;
#endif

  if (!nptr || !*nptr)
    panic("strtoll() passed a NULL string");

  s = nptr;

  /* skip white space */
  while (isspace((int)(*s)))
    ++s;
  if (*s == '\0')
    goto noconv;

  if (base == 0)
    {
      if (s[0] == '0' && toupper(s[1]) == 'X')
	base = 16;
      else
	base = 10;
    }

  if (base <= 1 || base > 36)
    panic("bogus base: %d", base);

  if (base == 16 && s[0] == '0' && toupper(s[1]) == 'X')
    s += 2;

  /* save the pointer so we can check later if anything happened */
  save = s;

  cutoff = ULL_MAX / (unsigned long int) base;
  cutlim = ULL_MAX % (unsigned long int) base;

  overflow = 0;
  i = 0;
  for (c = *s; c != '\0'; c = *++s)
    {
      if (isdigit (c))
        c -= '0';
      else if (isalpha (c))
        c = toupper(c) - 'A' + 10;
      else
        break;
      if (c >= base)
        break;

      /* check for overflow */
      if (i > cutoff || (i == cutoff && c > cutlim))
        overflow = 1;
      else
        {
          i *= (unsigned long int) base;
          i += c;
        }
    }

  /* check if anything actually happened */
  if (s == save)
    goto noconv;

  /* store in ENDP the address of one character past the last character
     we converted */
  if (endp != NULL)
    *endp = (char *) s;

  if (overflow)
    {
      errno = ERANGE;
      return ULL_MAX;
    }
  else
    {
      errno = 0;

      /* return the result of the appropriate sign */
      return i;
    }

noconv:
  /* there was no number to convert */
  if (endp != NULL)
    *endp = (char *) nptr;
  return 0;
}

#ifdef TESTIT
void
testit(char *s)
{
  char buf[128];
  qword_t qval;

  qval = myatoq(s, NULL, 10);

  myqtoa(qval, "%x", buf, NULL);
  fprintf(stderr, "x:    %s\n", buf);
  myqtoa(qval, "%16x", buf, NULL);
  fprintf(stderr, "16x:  %s\n", buf);
  myqtoa(qval, "%016x", buf, NULL);
  fprintf(stderr, "016x: %s\n", buf);
  myqtoa(qval, "0x%016x", buf, NULL);
  fprintf(stderr, "0x016x: %s\n", buf);
  myqtoa(qval, "0x%08x", buf, NULL);
  fprintf(stderr, "0x08x: %s\n", buf);

  myqtoa(qval, "%d", buf, NULL);
  fprintf(stderr, "d:    %s\n", buf);
  myqtoa(qval, "%22d", buf, NULL);
  fprintf(stderr, "22d:  %s\n", buf);
  myqtoa(qval, "%022d", buf, NULL);
  fprintf(stderr, "022d: %s\n", buf);

  myqtoa(qval, "%u", buf, NULL);
  fprintf(stderr, "u:    %s\n", buf);

  myqtoa(qval, "%o", buf, NULL);
  fprintf(stderr, "o:    %s\n", buf);
}

void
stestit(char *s)
{
  char buf[128];
  sqword_t sqval;

  sqval = myatosq(s, NULL, 10);

  myqtoa(sqval, "%x", buf, NULL);
  fprintf(stderr, "x: %s\n", buf);

  myqtoa(sqval, "%d", buf, NULL);
  fprintf(stderr, "d: %s\n", buf);

  myqtoa(sqval, "%u", buf, NULL);
  fprintf(stderr, "u: %s\n", buf);
}

void
xtestit(char *s)
{
  char buf[128];
  qword_t qval;

  qval = myatoq(s, NULL, 16);

  myqtoa(qval, "%x", buf, NULL);
  fprintf(stderr, "x: %s\n", buf);

  myqtoa(qval, "%d", buf, NULL);
  fprintf(stderr, "d: %s\n", buf);

  myqtoa(qval, "%u", buf, NULL);
  fprintf(stderr, "u: %s\n", buf);
}
#endif /* TESTIT */

#endif /* HOST_HAS_QWORD */

#ifdef GZIP_PATH

static struct {
  char *type;
  char *ext;
  char *cmd;
} gzcmds[] = {
  /* type */	/* extension */		/* command */
  { "r",	".gz",			"%s -dc %s" },
  { "rb",	".gz",			"%s -dc %s" },
  { "r",	".Z",			"%s -dc %s" },
  { "rb",	".Z",			"%s -dc %s" },
  { "w",	".gz",			"%s > %s" },
  { "wb",	".gz",			"%s > %s" }
};

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
FILE *
gzopen(char *fname, char *type)
{
  int i;
  char *cmd = NULL, *ext;
  FILE *fd;
  char str[2048];

  /* get the extension */
  ext = mystrrchr(fname, '.');

  /* check if extension indicates compressed file */
  if (ext != NULL && *ext != '\0')
    {
      for (i=0; i < N_ELT(gzcmds); i++)
	{
	  if (!strcmp(gzcmds[i].type, type) && !strcmp(gzcmds[i].ext, ext))
	    {
	      cmd = gzcmds[i].cmd;
	      break;
	    }
	}
    }

  if (!cmd)
    {
      /* open file */
      fd = fopen(fname, type);
    }
  else
    {
      /* open pipe to compressor/decompressor */
      sprintf(str, cmd, GZIP_PATH, fname);
      fd = popen(str, type);
    }

  return fd;
}

/* close compressed stream */
void
gzclose(FILE *fd)
{
  /* attempt pipe close, otherwise file close */
  if (pclose(fd) == -1)
    fclose(fd);
}

#else /* !GZIP_PATH */

FILE *
gzopen(char *fname, char *type)
{
  return fopen(fname, type);
}

void
gzclose(FILE *fd)
{
  fclose(fd);
}

#endif /* GZIP_PATH */
