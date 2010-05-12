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

#ifndef SSB_RS_LINK_H
#define SSB_RS_LINK_H

/*
 * RS_LINK defs and decls
 */

#include "ssb_host.h"
#include "ssb_machine.h"
#include "ssb_ruu.h"

//: A reservation station link
//
// a reservation station link: this structure links elements of a RUU
// reservation station list; used for ready instruction queue, event
// queue, and output dependency lists; each RS_LINK node contains a
// pointer to the RUU entry it references along with an instance tag,
// the RS_LINK is only valid if the instruction instance tag matches
// the instruction RUU entry instance tag; this strategy allows
// entries in the RUU can be squashed and reused without updating the
// lists that point to it, which significantly improves the
// performance of (all to frequent) squash events
//
//!SEC:ssBack
struct RS_link {
  RS_link *next;			/* next entry in list */
  struct RUU_station *rs;		/* referenced RUU resv station */
  INST_TAG_TYPE tag;			/* inst instance sequence number */
  union {
    tick_t when;			/* time stamp of entry (for eventq) */
    INST_SEQ_TYPE seq;			/* inst sequence */
    int opnum;				/* input/output operand number */
  } x;
};

class RS_link_list {
  RS_link* head;
public:
  RS_link_list(int nlinks);
  RS_link *RSLINK_NEW( struct RUU_station *rs )
  { struct RS_link *n_link;
    if (!head) panic("out of rs links");
    n_link = head;
    head = head->next;
    n_link->next = NULL;
    n_link->rs = rs; 
    n_link->tag = n_link->rs->tag;
    return n_link;
  };
  void RSLINK_FREE( RS_link *link) 
  {  RS_link *f_link = link;
     f_link->rs = NULL;
     f_link->tag = 0;
     f_link->next = head;
     head = f_link;
  };
  void RSLINK_FREE_LIST( RS_link *link)
  {  RS_link *fl_link, *fl_link_next;
     for (fl_link=link; fl_link; fl_link=fl_link_next)
       {
	 fl_link_next = fl_link->next;
	 RSLINK_FREE(fl_link);
       }
  };
  RS_link RSLINK_NULL;
};

/* create and initialize an RS link */
#define RSLINK_INIT(RSL, RS)						\
  ((RSL).next = NULL, (RSL).rs = (RS), (RSL).tag = (RS)->tag)

#if 0
/* non-zero if RS link is NULL */
#define RSLINK_IS_NULL(LINK)            ((LINK)->rs == NULL)
#endif

/* non-zero if RS link is to a valid (non-squashed) entry */
#define RSLINK_VALID(LINK)              ((LINK)->tag == (LINK)->rs->tag)

/* extra RUU reservation station pointer */
#define RSLINK_RS(LINK)                 ((LINK)->rs)

#if 0
/* get a new RS link record */
#define RSLINK_NEW(DST, RS)						\
  { struct RS_link *n_link;						\
    if (!RS_link::rslink_free_list)					\
      panic("out of rs links");						\
    n_link = RS_link::rslink_free_list;					\
    RS_link::rslink_free_list = RS_link::rslink_free_list->next;	\
    n_link->next = NULL;						\
    n_link->rs = (RS); n_link->tag = n_link->rs->tag;			\
    (DST) = n_link;							\
  }

/* free an RS link record */
#define RSLINK_FREE(LINK)						\
  {  RS_link *f_link = (LINK);					\
     f_link->rs = NULL; f_link->tag = 0;				\
     f_link->next = RS_link::rslink_free_list;				\
     RS_link::rslink_free_list = f_link;				\
  }

/* FIXME: could this be faster!!! */
/* free an RS link list */
#define RSLINK_FREE_LIST(LINK)						\
  {  RS_link *fl_link, *fl_link_next;				\
     for (fl_link=(LINK); fl_link; fl_link=fl_link_next)		\
       {								\
	 fl_link_next = fl_link->next;					\
	 RSLINK_FREE(fl_link);						\
       }								\
  }
#endif


#endif
