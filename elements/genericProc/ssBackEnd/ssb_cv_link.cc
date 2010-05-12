
#include "ssb_cv_link.h"
#include "ssb_sim-outorder.h"
#include "ssb_machine.h"
#include "ssb_ruu.h"

//!IGNORE:
CV_link CVLINK_NULL = { NULL, 0 };

/* initialize the create vector */
void CV_link::cv_init(convProc *p)
{
  int i;

  /* initially all registers are valid in the architected register file,
     i.e., the create vector entry is CVLINK_NULL */
  for (i=0; i < MD_TOTAL_REGS+2; i++)
    {
      p->create_vector[i] = CVLINK_NULL;
      p->create_vector_rt[i] = 0;
      p->spec_create_vector[i] = CVLINK_NULL;
      p->spec_create_vector_rt[i] = 0;
    }
  
  /* all create vector entries are non-speculative */
  BITMAP_CLEAR_MAP(p->use_spec_cv, CV_BMAP_SZ);
}


//: dump the contents of the create vector 
void CV_link::cv_dump(FILE *stream, convProc *p)	
{
  int i;
  CV_link ent;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** create vector state **\n");

  for (i=0; i < MD_TOTAL_REGS; i++)
    {
      ent = CREATE_VECTOR_P(p, i);
      if (!ent.rs)
	fprintf(stream, "[cv%02d]: from architected reg file\n", i);
      else
	fprintf(stream, "[cv%02d]: from %s, idx: %d\n",
		i, (ent.rs->in_LSQ ? "LSQ" : "RUU"),
		(int)(ent.rs - (ent.rs->in_LSQ ? p->LSQ : p->RUU)));
    }
}

