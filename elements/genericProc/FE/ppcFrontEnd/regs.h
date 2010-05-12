/*
 * regs.h - architected register state interfaces
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
 *
 */

#ifndef REGS_H
#define REGS_H

#include "host.h"
#include "misc.h"
#include "ppcMachine.h"
#include "sst/boost.h"

/*
 * This module implements the PowerPC architected register state, which
 * includes integer and floating point registers and miscellaneous registers.
 * The architected register state is as follows:
 *
 */

/* This is the structure that describes the register set for the PowerPC.
To see why these registers are here refer to the PowerPC users manual
which describes the registers that are seen in the user mode*/

//: PPC Special Registers
//!SEC:ppcFront
struct ppc_regs_t {
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_NVP(regs_C);
    ar & BOOST_SERIALIZATION_NVP(regs_L);
    ar & BOOST_SERIALIZATION_NVP(regs_CNTR);
    ar & BOOST_SERIALIZATION_NVP(regs_TBR);
  }
#endif
  //: control register file 
  md_ctrl_t regs_C;
  //: Link regsiter 
  md_link_t regs_L;
  //: Counter register 
  md_ctr_t  regs_CNTR;
  //: Time base register 
  md_addr_t regs_TBR;   
};

/* create a register file */
ppc_regs_t *regs_create(void);

/* initialize architected register state */
void
regs_init( ppc_regs_t *regs);		/* register file to initialize */

/* dump all architected register state values to output stream STREAM */
/*void
	  FILE *stream);		*/
/* destroy a register file */
/*void
regs_destroy(struct regs_t *regs);	
*/
#endif /* REGS_H */
