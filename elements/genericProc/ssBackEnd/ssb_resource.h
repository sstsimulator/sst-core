/*
 * resource.h - resource manager interfaces
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
 * $Id: ssb_resource.h,v 1.2 2007-01-24 21:34:03 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2006/01/31 16:35:50  afrodri
 * first entry
 *
 * Revision 1.4  2005/08/16 21:12:55  arodrig6
 * changes for docs
 *
 * Revision 1.3  2004/11/16 04:17:46  arodrig6
 * added more documentation
 *
 * Revision 1.2  2004/08/10 22:55:35  arodrig6
 * works, except for the speculative stuff, caches, and multiple procs
 *
 * Revision 1.1  2004/08/05 23:51:45  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.2  2000/03/23 02:41:36  karu
 * Merged differences with ramdass's machine.def and differences
 * made to make sim-outorder work.
 *
 * Revision 1.1.1.1  2000/03/07 05:15:18  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:52  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.4  1998/08/27 15:54:19  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.3  1997/03/11  01:26:49  taustin
 * updated copyright
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef SSB_RESOURCE_H
#define SSB_RESOURCE_H

#include <stdio.h>

/* maximum number of resource classes supported */
#define MAX_RES_CLASSES	 2048	

/* maximum number of resource instances for a class supported */
#define MAX_INSTS_PER_CLASS	8

//: resource template 
//!SEC:ssBack
struct res_template {
  int Rclass;				/* matching resource class: insts
					   with this resource class will be
					   able to execute on this unit */
  int oplat;				/* operation latency: cycles until
					   result is ready for use */
  int issuelat;			/* issue latency: number of cycles
				   before another operation can be
				   issued on this resource */
  struct res_desc *master;		/* master resource record */
};

//: resource descriptor 
//!SEC:ssBack
struct res_desc {
  const char *name;				/* name of functional unit */
  int quantity;				/* total instances of this unit */
  int busy;				/* non-zero if this unit is busy */
  res_template x[MAX_RES_CLASSES];
};

//: resource pool: one entry per resource instance 
//!SEC:ssBack
struct res_pool {
  char *name;				/* pool name */
  int num_resources;			/* total number of res instances */
   res_desc *resources;		/* resource instances */
  /* res class -> res template mapping table, lists are NULL terminated */
  int nents[MAX_RES_CLASSES];
   res_template *table[MAX_RES_CLASSES][MAX_INSTS_PER_CLASS];
};

/* create a resource pool */
 res_pool *res_create_pool(const char *name,  const res_desc *pool, int ndesc);

//: get resource
//
// get a free resource from resource pool POOL that can execute a
// operation of class CLASS, returns a pointer to the resource
// template, returns NULL, if there are currently no free resources
// available, follow the MASTER link to the master resource
// descriptor; NOTE: caller is responsible for reseting the busy flag
// in the beginning of the cycle when the resource can once again
// accept a new operation
 res_template *res_get( res_pool *pool, int Rclass);

/* dump the resource pool POOL to stream STREAM */
void res_dump( res_pool *pool, FILE *stream);

#endif /* RESOURCE_H */
