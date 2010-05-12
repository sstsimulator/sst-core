/*
 * eventq.h - event queue manager interfaces
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
 * $Id: ssb_eventq.h,v 1.1.1.1 2006-01-31 16:35:49 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2004/11/16 04:17:46  arodrig6
 * added more documentation
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.1.1.1  2000/03/07 05:15:16  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:50  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.4  1998/08/27 08:28:14  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.3  1997/03/11  01:12:30  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdio.h>

#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_bitmap.h"

/* This module implements a time ordered event queue.  Users insert
 *
 */

/* event actions */
enum eventq_action {
  EventSetBit,			/* set a bit: int *, bit # */
  EventClearBit,		/* clear a bit: int *, bit # */
  EventSetFlag,			/* set a flag: int *, value */
  EventAddOp,			/* add a value to a summand */
  EventCallback,		/* call event handler: fn * */
};

/* ID zero (0) is unused */
typedef unsigned int EVENTQ_ID_TYPE;

//: event descriptor 
//!SEC:ssBack
struct eventq_desc {
  struct eventq_desc *next;	/* next event in the sorted list */
  SS_TIME_TYPE when;		/* time to schedule write back event */
  EVENTQ_ID_TYPE id;		/* unique event ID */
  enum eventq_action action;	/* action on event occurrance */
  union eventq_data {
    struct {
      BITMAP_ENT_TYPE *bmap;	/* bitmap to access */
      int sz;			/* bitmap size */
      int bitnum;		/* bit to set */
    } bit;
    struct {
      int *pflag;		/* flag to set */
      int value;
    } flag;
    struct {
      int *summand;		/* value to add to */
      int addend;		/* amount to add */
    } addop;
    struct {
      void (*fn)(SS_TIME_TYPE time, int arg);	/* function to call */
      int arg;			/* argument to pass */
    } callback;
  } data;
};

/* initialize the event queue module, MAX_EVENT is the most events allowed
   pending, pass a zero if there is no limit */
void eventq_init(int max_events);

/* schedule an action that occurs at WHEN, action is visible at WHEN,
   and invisible before WHEN */
EVENTQ_ID_TYPE eventq_queue_setbit(SS_TIME_TYPE when,
				   BITMAP_ENT_TYPE *bmap, int sz, int bitnum);
EVENTQ_ID_TYPE eventq_queue_clearbit(SS_TIME_TYPE when, BITMAP_ENT_TYPE *bmap,
				     int sz, int bitnum);
EVENTQ_ID_TYPE eventq_queue_setflag(SS_TIME_TYPE when,
				    int *pflag, int value);
EVENTQ_ID_TYPE eventq_queue_addop(SS_TIME_TYPE when,
				  int *summand, int addend);
EVENTQ_ID_TYPE eventq_queue_callback(SS_TIME_TYPE when,
				     void (*fn)(SS_TIME_TYPE time, int arg),
				     int arg);

/* execute an event immediately, returns non-zero if the event was
   located an deleted */
int eventq_execute(EVENTQ_ID_TYPE id);

/* remove an event from the eventq, action is never performed, returns
   non-zero if the event was located an deleted */
int eventq_remove(EVENTQ_ID_TYPE id);

/* service all events in order of occurrance until and at NOW */
void eventq_service_events(SS_TIME_TYPE now);

void eventq_dump(FILE *stream);

#endif /* EVENT_H */
