#include "ssb_sim-outorder.h"

//: dump the contents of the ready queue 
void
convProc::readyq_dump(FILE *stream)			/* output stream */
{
  RS_link *link;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** ready queue state **\n");

  for (link = ready_queue; link != NULL; link = link->next)
    {
      /* is entry still valid? */
      if (RSLINK_VALID(link))
	{
	  RUU_station *rs = RSLINK_RS(link);

	  ruu_dumpent(rs, rs - (rs->in_LSQ ? LSQ : RUU),
		      stream, /* header */TRUE);
	}
    }
}

//: dump the contents of the event queue 
void
convProc::eventq_dump(FILE *stream)			/* output stream */
{
  RS_link *ev;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** event queue state **\n");

  for (ev = event_queue; ev != NULL; ev = ev->next)
    {
      /* is event still valid? */
      if (RSLINK_VALID(ev))
	{
	  RUU_station *rs = RSLINK_RS(ev);

	  fprintf(stream, "idx: %2d: @ %.0f\n",
		  (int)(rs - (rs->in_LSQ ? LSQ : RUU)), (double)ev->x.when);
	  ruu_dumpent(rs, rs - (rs->in_LSQ ? LSQ : RUU),
		      stream, /* !header */FALSE);
	}
    }
}

//: dump the contents of the RUU 
static void
ruu_dumpent(struct RUU_station *rs,		/* ptr to RUU station */
	    int index,				/* entry index */
	    FILE *stream,			/* output stream */
	    int header)				/* print header? */
{
  if (!stream)
    stream = stderr;

  if (header)
    fprintf(stream, "idx: %2d: opcode: %s, inst: `",
	    index, MD_OP_NAME(rs->op));
  else
    fprintf(stream, "       opcode: %s, inst: `",
	    MD_OP_NAME(rs->op));
  md_print_insn(rs->IR, rs->PC, stream);
  fprintf(stream, "'\n");
  myfprintf(stream, "         PC: 0x%08p, NPC: 0x%08p (pred_PC: 0x%08p)\n",
	    rs->PC, rs->next_PC, rs->pred_PC);
  fprintf(stream, "         in_LSQ: %s, ea_comp: %s, recover_inst: %s\n",
	  rs->in_LSQ ? "t" : "f",
	  rs->ea_comp ? "t" : "f",
	  rs->recover_inst ? "t" : "f");
  myfprintf(stream, "         spec_mode: %s, addr: 0x%08p, tag: 0x%08x\n",
	    rs->spec_mode ? "t" : "f", rs->addr, rs->tag);
  fprintf(stream, "         seq: 0x%08x, ptrace_seq: 0x%08x\n",
	  rs->seq, rs->ptrace_seq);
  fprintf(stream, "         queued: %s, issued: %s, completed: %s\n",
	  rs->queued ? "t" : "f",
	  rs->issued ? "t" : "f",
	  rs->completed ? "t" : "f");
  fprintf(stream, "         operands ready: %s\n",
	  OPERANDS_READY(rs) ? "t" : "f");
}


//: dump the contents of the RUU 
void covnProc::ruu_dump(FILE *stream)	* output stream */
{
  int num, head;
  RUU_station *rs;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** RUU state **\n");
  fprintf(stream, "RUU_head: %d, RUU_tail: %d\n", RUU_head, RUU_tail);
  fprintf(stream, "RUU_num: %d\n", RUU_num);

  num = RUU_num;
  head = RUU_head;
  while (num)
    {
      rs = &RUU[head];
      ruu_dumpent(rs, rs - RUU, stream, /* header */TRUE);
      head = (head + 1) % RUU_size;
      num--;
    }
}

//: dump the contents of the LSQ 
static void
convProc::lsq_dump(FILE *stream)		/* output stream */
{
  int num, head;
  RUU_station *rs;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** LSQ state **\n");
  fprintf(stream, "LSQ_head: %d, LSQ_tail: %d\n", LSQ_head, LSQ_tail);
  fprintf(stream, "LSQ_num: %d\n", LSQ_num);

  num = LSQ_num;
  head = LSQ_head;
  while (num)
    {
      rs = &LSQ[head];
      ruu_dumpent(rs, rs - LSQ, stream, /* header */TRUE);
      head = (head + 1) % LSQ_size;
      num--;
    }
}

//: dump speculative register state 
static void
rspec_dump(FILE *stream)			/* output stream */
{
  int i;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** speculative register contents **\n");

  fprintf(stream, "spec_mode: %s\n", spec_mode ? "t" : "f");

  /* dump speculative integer regs */
  for (i=0; i < MD_NUM_IREGS; i++)
    {
      if (BITMAP_SET_P(use_spec_R, R_BMAP_SZ, i))
	{
	  md_print_ireg(spec_regs_R, i, stream);
	  fprintf(stream, "\n");
	}
    }

  /* dump speculative FP regs */
  for (i=0; i < MD_NUM_FREGS; i++)
    {
      if (BITMAP_SET_P(use_spec_F, F_BMAP_SZ, i))
	{
	  md_print_fpreg(spec_regs_F, i, stream);
	  fprintf(stream, "\n");
	}
    }

  /* dump speculative CTRL regs */
  for (i=0; i < MD_NUM_CREGS; i++)
    {
      if (BITMAP_SET_P(use_spec_C, C_BMAP_SZ, i))
	{
	  md_print_creg(spec_regs_C, i, stream);
	  fprintf(stream, "\n");
	}
    }

#ifdef TARGET_PPC
  /* dump speculative link and counter register state*/
  fprintf(stream, "LINK: 0x%08x\n", spec_regs_L);
  fprintf(stream, "CNTR: 0x%08x\n", spec_regs_CNTR);
#endif

  /* dump more speculative register states */
}


//: dump speculative memory state 
void
convProc::mspec_dump(FILE *stream)			/* output stream */
{
  int i;
  spec_mem_ent *ent;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** speculative memory contents **\n");

  fprintf(stream, "spec_mode: %s\n", spec_mode ? "t" : "f");

  for (i=0; i<STORE_HASH_SIZE; i++)
    {
      /* dump contents of all hash table buckets */
      for (ent=store_htable[i]; ent; ent=ent->next)
	{
	  myfprintf(stream, "[0x%08p]: %12.0f/0x%08x:%08x\n",
		    ent->addr, (double)(*((double *)ent->data)),
		    *((unsigned int *)&ent->data[0]),
		    *(((unsigned int *)&ent->data[0]) + 1));
	}
    }
}


//: dump contents of fetch stage registers and fetch queue 
void convProc::fetch_dump(FILE *stream)			/* output stream */
{
  int num, head;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** fetch stage state **\n");

  fprintf(stream, "spec_mode: %s\n", spec_mode ? "t" : "f");
  myfprintf(stream, "pred_PC: 0x%08p, recover_PC: 0x%08p\n",
	    pred_PC, recover_PC);
  myfprintf(stream, "fetch_regs_PC: 0x%08p, fetch_pred_PC: 0x%08p\n",
	    fetch_regs_PC, fetch_pred_PC);
  fprintf(stream, "\n");

  fprintf(stream, "** fetch queue contents **\n");
  fprintf(stream, "fetch_num: %d\n", fetch_num);
  fprintf(stream, "fetch_head: %d, fetch_tail: %d\n",
	  fetch_head, fetch_tail);

  num = fetch_num;
  head = fetch_head;
  while (num)
    {
      fprintf(stream, "idx: %2d: inst: `", head);
      md_print_insn(fetch_data[head].IR, fetch_data[head].regs_PC, stream);
      fprintf(stream, "'\n");
      myfprintf(stream, "         regs_PC: 0x%08p, pred_PC: 0x%08p\n",
		fetch_data[head].regs_PC, fetch_data[head].pred_PC);
      head = (head + 1) & (ruu_ifq_size - 1);
      num--;
    }
}
