// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "ssb_sim-outorder.h"
#include "ssb_rs_link.h"

/*
 * the execution unit event queue implementation follows, the event queue
 * indicates which instruction will complete next, the writeback handler
 * drains this queue. ssb_sim-outorder-eventq.cc
 */

//: init event Q
// initialize the event queue structures.
//
// the event queue indicates which instruction will complete next, the
//  writeback handler drains this queue.
void
convProc::eventq_init(void)
{
  event_queue = NULL;
}

//: Insert into eventq
//
// insert an event for RS into the event queue, event queue is sorted
// from earliest to latest event, event and associated side-effects
// will be apparent at the start of TimeStamp WHEN 
void
convProc::eventq_queue_event( RUU_station *rs, tick_t when)
{
   RS_link *prev, *ev, *new_ev;

  if (rs->completed)
    panic("event completed");

  if ((unsigned long long)when <= TimeStamp()) 
    panic("event occurred in the past");

  /* get a free event record */
  new_ev = rs_free_list.RSLINK_NEW(rs);
  new_ev->x.when = when;

  /* locate insertion point */
  for (prev=NULL, ev=event_queue;
       ev && ev->x.when < when;
       prev=ev, ev=ev->next);

  if (prev)
    {
      /* insert middle or end */
      new_ev->next = prev->next;
      prev->next = new_ev;
    }
  else
    {
      /* insert at beginning */
      new_ev->next = event_queue;
      event_queue = new_ev;
    }
}

//: get next event frome ventq
//
// return the next event that has already occurred, returns NULL when
// no remaining events or all remaining events are in the future
 RUU_station *
convProc::eventq_next_event(void)
{
   RS_link *ev;

  if (event_queue && (unsigned long long)event_queue->x.when <= TimeStamp())
    {
      /* unlink and return first event on priority list */
      ev = event_queue;
      event_queue = event_queue->next;

      /* event still valid? */
      if (RSLINK_VALID(ev))
	{
	   RUU_station *rs = RSLINK_RS(ev);

	  /* reclaim event record */
	   rs_free_list.RSLINK_FREE(ev);

	  /* event is valid, return resv station */
	  return rs;
	}
      else
	{
	  /* reclaim event record */
	  rs_free_list.RSLINK_FREE(ev);

	  /* receiving inst was squashed, return next event */
	  return eventq_next_event();
	}
    }
  else
    {
      /* no event or no event is ready */
      return NULL;
    }
}
