#include "ssb_sim-outorder.h"
#include "ssb_rs_link.h"

//: register input deps
//
// link RS onto the output chain number of whichever operation will
// next create the architected register value IDEP_NAME
void
convProc::ruu_link_idep(struct RUU_station * const rs,	/* rs station to link */
			const int idep_num,	/* input dependence number */
			const int idep_name)	/* input register name */
{
   CV_link head;
   RS_link *link;

  /* any dependence? */
  if (idep_name == NA)
    {
      /* no input dependence for this input slot, mark operand as ready */
      rs->idep_ready[idep_num] = TRUE;
      return;
    }

  /* locate creator of operand */
  head = CREATE_VECTOR(idep_name);

  /* any creator? */
  if (!head.rs)
    {
      /* no active creator, use value available in architected reg file,
         indicate the operand is ready for use */
      rs->idep_ready[idep_num] = TRUE;
      return;
    }
  /* else, creator operation will make this value sometime in the future */

  /* indicate value will be created sometime in the future, i.e., operand
     is not yet ready for use */
  rs->idep_ready[idep_num] = FALSE;

  /* link onto creator's output list of dependant operand */
  link = rs_free_list.RSLINK_NEW(rs); link->x.opnum = idep_num;
  link->next = head.rs->odep_list[head.odep_num];
  head.rs->odep_list[head.odep_num] = link;
}

//: register output deps
//
// make RS the creator of architected register ODEP_NAME 
void
convProc::ruu_install_odep(struct RUU_station *rs,/* creating RUU station */
			   int odep_num,	/* output operand number */
			   int odep_name)	/* output register name */
{
  CV_link cv;

  /* any dependence? */
  if (odep_name == NA)
    {
      /* no value created */
      rs->onames[odep_num] = NA;
      return;
    }
  /* else, create a RS_NULL terminated output chain in create vector */

  /* record output name, used to update create vector at completion */
  rs->onames[odep_num] = odep_name;

  /* initialize output chain to empty list */
  rs->odep_list[odep_num] = NULL;

  /* indicate this operation is latest creator of ODEP_NAME */
  CVLINK_INIT(cv, rs, odep_num);
  SET_CREATE_VECTOR(odep_name, cv);
}

