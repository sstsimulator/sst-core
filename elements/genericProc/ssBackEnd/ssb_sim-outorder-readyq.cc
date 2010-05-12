#include "ssb_sim-outorder.h"
#include "ssb_rs_link.h"

/*
 * the ready instruction queue implementation follows, the ready instruction
 * queue indicates which instruction have all of there *register* dependencies
 * satisfied, instruction will issue when 1) all memory dependencies for
 * the instruction have been satisfied (see lsq_refresh() for details on how
 * this is accomplished) and 2) resources are available; ready queue is fully
 * constructed each cycle before any operation is issued from it -- this
 * ensures that instruction issue priorities are properly observed; NOTE:
 * RS_LINK nodes are used for the event queue list so that it need not be
 * updated during squash events
 */



//: initialize the event queue structures 
void
convProc::readyq_init(void)
{
  ready_queue = NULL;
}

/* insert ready node into the ready list using ready instruction scheduling
   policy; currently the following scheduling policy is enforced:

     memory and long latency operands, and branch instructions first

   then

     all other instructions, oldest instructions first

  this policy works well because branches pass through the machine quicker
  which works to reduce branch misprediction latencies, and very long latency
  instructions (such loads and multiplies) get priority since they are very
  likely on the program's critical path */
void
convProc::readyq_enqueue(RUU_station *rs)	/* RS to enqueue */
{
   RS_link *prev, *node, *new_node;

  /* node is now queued */
  if (rs->queued)
    panic("node is already queued");
  rs->queued = TRUE;

  /* get a free ready list node */
  new_node = rs_free_list.RSLINK_NEW(rs);
  new_node->x.seq = rs->seq;

  /* locate insertion point */
  //if (rs->in_LSQ || (rs->IR->specificOp() & (F_LONGLAT|F_CTRL)))
  if (0) 
    {
      /* insert loads/stores and long latency ops at the head of the queue */
      prev = NULL;
      node = ready_queue;
    }
  else
    {
      /* otherwise insert in program order (earliest seq first) */
      for (prev=NULL, node=ready_queue;
	   node && node->x.seq < rs->seq;
	   prev=node, node=node->next);
    }

  if (prev)
    {
      /* insert middle or end */
      new_node->next = prev->next;
      prev->next = new_node;
    }
  else
    {
      /* insert at beginning */
      new_node->next = ready_queue;
      ready_queue = new_node;
    }
}
