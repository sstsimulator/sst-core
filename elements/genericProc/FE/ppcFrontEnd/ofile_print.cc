/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * This file intention is to beable to print the structures in an object file
 * and handle problems with reguard to alignment and bytesex.  The goal is to
 * print as much as possible even when things are truncated or trashed.  Both
 * a verbose (symbolic) and non-verbose modes are supported to aid in seeing
 * the values even if they are not correct.  As much as possible strict checks
 * on values of fields for correctness should be done (such as proper alignment)
 * and notations on errors should be printed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <ar.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/reloc.h>
/*#include "stuff/ofile.h"
#include "stuff/allocate.h"
#include "stuff/errors.h"
#include "stuff/guess_short_name.h"*/
#include "ofile_print.h"

/* <mach/loader.h> */
/* The maximum section alignment allowed to be specified, as a power of two */
#define MAXSECTALIGN		15 /* 2**15 or 0x8000 */




/*
 * Print the mach header.  It is assumed that the structure pointed to by mh
 * is aligned correctly and in the host byte sex.  In this way it is up to the
 * caller to determine he has a mach_header and what byte sex it is and get it
 * aligned in the host byte sex for this routine.
 */
void
print_mach_header(struct mach_header *mh, bool verbose)
{
    unsigned long flags;

	printf("Mach header\n");
	printf("      magic cputype cpusubtype   filetype ncmds sizeofcmds "
	       "     flags\n");
	if(verbose){
	    if(mh->magic == MH_MAGIC)
		printf("   MH_MAGIC");
	    else
		printf(" 0x%08x", (unsigned int)mh->magic);
	    switch(mh->cputype){
	    case CPU_TYPE_POWERPC:
	      printf("     PPC");
		switch(mh->cpusubtype){
		case CPU_SUBTYPE_POWERPC_ALL:
		    printf("        ALL");
		    break;
		case CPU_SUBTYPE_POWERPC_601:
		    printf("     ppc601");
		    break;
		case CPU_SUBTYPE_POWERPC_602:
		    printf("     ppc602");
		    break;
		case CPU_SUBTYPE_POWERPC_603:
		    printf("     ppc603");
		    break;
		case CPU_SUBTYPE_POWERPC_603e:
		    printf("    ppc603e");
		    break;
		case CPU_SUBTYPE_POWERPC_603ev:
		    printf("   ppc603ev");
		    break;
		case CPU_SUBTYPE_POWERPC_604:
		    printf("     ppc604");
		    break;
		case CPU_SUBTYPE_POWERPC_604e:
		    printf("    ppc604e");
		    break;
		case CPU_SUBTYPE_POWERPC_620:
		    printf("     ppc620");
		    break;
		case CPU_SUBTYPE_POWERPC_750:
		    printf("     ppc750");
		    break;
		case CPU_SUBTYPE_POWERPC_7400:
		    printf("    ppc7400");
		    break;
		case CPU_SUBTYPE_POWERPC_7450:
		    printf("    ppc7450");
		    break;
		case CPU_SUBTYPE_POWERPC_970:
		    printf("     ppc970");
		    break;
		default:
		    printf(" %10d", mh->cpusubtype);
		    break;
		}
		break;
	    default:
		printf(" %7d %10d", mh->cputype, mh->cpusubtype);
		break;
	    }
	    switch(mh->filetype){
	    case MH_OBJECT:
		printf("     OBJECT");
		break;
	    case MH_EXECUTE:
		printf("    EXECUTE");
		break;
	    case MH_FVMLIB:
		printf("     FVMLIB");
		break;
	    case MH_CORE:
		printf("       CORE");
		break;
	    case MH_PRELOAD:
		printf("    PRELOAD");
		break;
	    case MH_DYLIB:
		printf("      DYLIB");
		break;
	    case MH_DYLIB_STUB:
		printf(" DYLIB_STUB");
		break;
	    case MH_DYLINKER:
		printf("   DYLINKER");
		break;
	    case MH_BUNDLE:
		printf("     BUNDLE");
		break;
	    default:
	      printf(" %10lu", (long unsigned int)mh->filetype);
		break;
	    }
	    printf(" %5lu %10lu", (long unsigned int)mh->ncmds, (long unsigned int)mh->sizeofcmds);
	    flags = mh->flags;
	    if(flags & MH_NOUNDEFS){
		printf("   NOUNDEFS");
		flags &= ~MH_NOUNDEFS;
	    }
	    if(flags & MH_INCRLINK){
		printf(" INCRLINK");
		flags &= ~MH_INCRLINK;
	    }
	    if(flags & MH_DYLDLINK){
		printf(" DYLDLINK");
		flags &= ~MH_DYLDLINK;
	    }
	    if(flags & MH_BINDATLOAD){
		printf(" BINDATLOAD");
		flags &= ~MH_BINDATLOAD;
	    }
	    if(flags & MH_PREBOUND){
		printf(" PREBOUND");
		flags &= ~MH_PREBOUND;
	    }
	    if(flags & MH_SPLIT_SEGS){
		printf(" SPLIT_SEGS");
		flags &= ~MH_SPLIT_SEGS;
	    }
	    if(flags & MH_LAZY_INIT){
		printf(" LAZY_INIT");
		flags &= ~MH_LAZY_INIT;
	    }
	    if(flags & MH_TWOLEVEL){
		printf(" TWOLEVEL");
		flags &= ~MH_TWOLEVEL;
	    }
	    if(flags & MH_FORCE_FLAT){
		printf(" FORCE_FLAT");
		flags &= ~MH_FORCE_FLAT;
	    }
	    if(flags & MH_NOMULTIDEFS){
		printf(" NOMULTIDEFS");
		flags &= ~MH_NOMULTIDEFS;
	    }
	    if(flags & MH_NOFIXPREBINDING){
		printf(" NOFIXPREBINDING");
		flags &= ~MH_NOFIXPREBINDING;
	    }
	    if(flags != 0 || mh->flags == 0)
		printf(" 0x%08x", (unsigned int)flags);
	    printf("\n");
	}
	else{
	    printf(" 0x%08x %7d %10d %10lu %5lu %10lu 0x%08x\n",
		   (unsigned int)mh->magic, mh->cputype, (int)mh->cpusubtype,
		   (long unsigned int)mh->filetype, (unsigned long)mh->ncmds, 
            (unsigned long) mh->sizeofcmds,
		   (unsigned int)mh->flags);
	}
}

