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

#ifndef SSB_CV_LINK_H
#define SSB_CV_LINK_H

#include <stdio.h>


class convProc;

//: an entry in the create vector 
//
// The create vector maps a logical register to a creator in the RUU
// (and specific output operand) or the architected register file (if
// RS_link is NULL)
//
//!SEC:ssBack
struct CV_link {
  static void cv_init(convProc*);
  static void cv_dump(FILE *stream, convProc*);
  struct RUU_station *rs;               /* creator's reservation station */
  int odep_num;                         /* specific output operand */
};

/* a NULL create vector entry */
extern CV_link CVLINK_NULL;

/* get a new create vector link */
#define CVLINK_INIT(CV, RS,ONUM)	((CV).rs = (RS), (CV).odep_num = (ONUM))

/* size of the create vector (one entry per architected register) */
#define CV_BMAP_SZ              (BITMAP_SIZE(MD_TOTAL_REGS+2))


#endif
