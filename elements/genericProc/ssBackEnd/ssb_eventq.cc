/*
 * eventq.c - event queue manager routines
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
 * $Id: ssb_eventq.cc,v 1.1.1.1 2006-01-31 16:35:49 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2005/08/16 21:12:55  arodrig6
 * changes for docs
 *
 * Revision 1.2  2005/06/28 21:21:22  arodrig6
 * changes for GCC 4.0 - mainly making simpleScalar tighter
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
 * Revision 1.4  1998/08/27 08:28:03  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.3  1997/03/11  01:12:18  taustin
 * updated copyright
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_machine.h"
#include "ssb_eventq.h"

int eventq_max_events;
int eventq_event_count;
eventq_desc *eventq_pending;
eventq_desc *eventq_free;

static EVENTQ_ID_TYPE next_ID = 1;

void
eventq_init(int max_events)
{
  eventq_max_events = max_events;
  eventq_event_count = 0;
  eventq_pending = NULL;
  eventq_free = NULL;
}

#define __QUEUE_EVENT(WHEN, ID, ACTION)					\
  eventq_desc *prev, *ev, *newe;					\
  /* get a free event descriptor */					\
  if (!eventq_free)							\
    {									\
      if (eventq_max_events && eventq_event_count >= eventq_max_events)	\
	panic("too many events");					\
      eventq_free = (eventq_desc*)calloc(1, sizeof(eventq_desc));	\
    }									\
  newe = eventq_free;							\
  eventq_free = eventq_free->next;					\
  /* plug in event data */						\
  newe->when = (WHEN); (ID) = newe->id = next_ID++; ACTION;		\
  /* locate insertion point */						\
  for (prev=NULL,ev=eventq_pending;					\
       ev && ev->when < when;						\
       prev=ev, ev=ev->next);						\
  /* insert new record */						\
  if (prev)								\
    {									\
      /* insert middle or end */					\
      newe->next = prev->next;						\
      prev->next = newe;							\
    }									\
  else									\
    {									\
      /* insert beginning */						\
      newe->next = eventq_pending;					\
      eventq_pending = newe;						\
    }

EVENTQ_ID_TYPE
eventq_queue_setbit(SS_TIME_TYPE when,
		    BITMAP_ENT_TYPE *bmap, int sz, int bitnum)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		newe->action = EventSetBit; newe->data.bit.bmap = bmap;	\
		newe->data.bit.sz = sz; newe->data.bit.bitnum = bitnum);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_clearbit(SS_TIME_TYPE when,
		      BITMAP_ENT_TYPE *bmap, int sz, int bitnum)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		newe->action = EventClearBit; newe->data.bit.bmap = bmap;	\
		newe->data.bit.sz = sz; newe->data.bit.bitnum = bitnum);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_setflag(SS_TIME_TYPE when, int *pflag, int value)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		newe->action = EventSetFlag;				\
		newe->data.flag.pflag = pflag; newe->data.flag.value = value);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_addop(SS_TIME_TYPE when, int *summand, int addend)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		newe->action = EventAddOp;				\
		newe->data.addop.summand = summand;			\
		newe->data.addop.addend = addend);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_callback(SS_TIME_TYPE when,
		      void (*fn)(SS_TIME_TYPE time, int arg), int arg)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		newe->action = EventCallback; newe->data.callback.fn = fn;\
		newe->data.callback.arg = arg);
  return id;
}

#define EXECUTE_ACTION(ev, now)						\
  /* execute action */							\
  switch (ev->action) {							\
  case EventSetBit:							\
    BITMAP_SET(ev->data.bit.bmap, ev->data.bit.sz, ev->data.bit.bitnum);\
    break;								\
  case EventClearBit:							\
    BITMAP_CLEAR(ev->data.bit.bmap, ev->data.bit.sz, ev->data.bit.bitnum);\
    break;								\
  case EventSetFlag:							\
    *ev->data.flag.pflag = ev->data.flag.value;				\
    break;								\
  case EventAddOp:							\
    *ev->data.addop.summand += ev->data.addop.addend;			\
    break;								\
  case EventCallback:							\
    (*ev->data.callback.fn)(now, ev->data.callback.arg);		\
    break;								\
  default:								\
    panic("bogus event action");					\
  }

/* execute an event immediately, returns non-zero if the event was
   located an deleted */
int
eventq_execute(EVENTQ_ID_TYPE id)
{
  eventq_desc *prev, *ev;

  for (prev=NULL,ev=eventq_pending; ev; prev=ev,ev=ev->next)
    {
      if (ev->id == id)
	{
	  if (prev)
	    {
	      /* middle of end of list */
	      prev->next = ev->next;
	    }
	  else /* !prev */
	    {
	      /* beginning of list */
	      eventq_pending = ev->next;
	    }

	  /* handle action, now is munged */
	  EXECUTE_ACTION(ev, 0);

	  /* put event on free list */
	  ev->next = eventq_free;
	  eventq_free = ev;

	  /* return success */
	  return TRUE;
	}
    }
  /* not found */
  return FALSE;
}

/* remove an event from the eventq, action is never performed, returns
   non-zero if the event was located an deleted */
int
eventq_remove(EVENTQ_ID_TYPE id)
{
  eventq_desc *prev, *ev;

  for (prev=NULL,ev=eventq_pending; ev; prev=ev,ev=ev->next)
    {
      if (ev->id == id)
	{
	  if (prev)
	    {
	      /* middle of end of list */
	      prev->next = ev->next;
	    }
	  else /* !prev */
	    {
	      /* beginning of list */
	      eventq_pending = ev->next;
	    }

	  /* put event on free list */
	  ev->next = eventq_free;
	  eventq_free = ev;

	  /* return success */
	  return TRUE;
	}
    }
  /* not found */
  return FALSE;
}

void
eventq_service_events(SS_TIME_TYPE now)
{
  while (eventq_pending && eventq_pending->when <= now)
    {
       eventq_desc *ev = eventq_pending;

      /* handle action */
      EXECUTE_ACTION(ev, now);

      /* return the event record to the free list */
      eventq_pending = ev->next;
      ev->next = eventq_free;
      eventq_free = ev;
  }
}

void
eventq_dump(FILE *stream)
{
  eventq_desc *ev;

  if (!stream)
    stream = stderr;

  fprintf(stream, "Pending Events: ");
  for (ev=eventq_pending; ev; ev=ev->next)
    {
      fprintf(stream, "@ %.0f:%s:",
	      (double)ev->when,
	      ev->action == EventSetBit ? "set bit"
	      : ev->action == EventClearBit ? "clear bit"
	      : ev->action == EventSetFlag ? "set flag"
	      : ev->action == EventAddOp ? "add operation"
	      : ev->action == EventCallback ? "call back"
	      : (abort(), ""));
      switch (ev->action) {
      case EventSetBit:
      case EventClearBit:
	fprintf(stream, "0x%p, %d, %d",
		ev->data.bit.bmap, ev->data.bit.sz, ev->data.bit.bitnum);
	break;
      case EventSetFlag:
	fprintf(stream, "0x%p, %d", ev->data.flag.pflag, ev->data.flag.value);
	break;
      case EventAddOp:
	fprintf(stream, "0x%p, %d",
		ev->data.addop.summand, ev->data.addop.addend);
	break;
      case EventCallback:
	fprintf(stream, "0x%p, %d",
		ev->data.callback.fn, ev->data.callback.arg);
	break;
      default:
	panic("bogus event action");
      }
      fprintf(stream, " ");
    }
}
