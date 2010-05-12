// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef FUCLASSES_H
#define FUCLASSES_H

/* function unit classes, update md_fu2name in individual front-ends if you update this definition */
enum md_fu_class {
  FUClass_NA = 0,	/* inst does not use a functional unit */
  IntALU=0x1,		/* integer ALU */
  IntMULT=0x2,		/* integer multiplier */
  IntDIV=0x4,		/* integer divider */
  FloatADD=0x8,		/* floating point adder/subtractor */
  FloatCMP=0x10,		/* floating point comparator */
  FloatCVT=0x20,		/* floating point<->integer converter */
  FloatMULT=0x40,		/* floating point multiplier */
  FloatDIV=0x80,		/* floating point divider */
  FloatSQRT=0x100,		/* floating point square root */
  RdPort=0x200,		/* memory read port */
  WrPort=0x400,		/* memory write port */
  NUM_FU_CLASSES=0x800	/* total functional unit classes */
};

#define F_ICOMP		0x00000001	/* integer computation */
#define F_FCOMP		0x00000002	/* FP computation */
#define F_CTRL		0x00000004	/* control inst */
#define F_UNCOND	0x00000008	/*   unconditional change */
#define F_COND		0x00000010	/*   conditional change */
#define F_MEM		0x00000020	/* memory access inst */
#define F_LOAD		0x00000040	/*   load inst */
#define F_STORE		0x00000080	/*   store inst */
#define F_DISP		0x00000100	/*   displaced (R+C) addr mode */
#define F_RR		0x00000200	/*   R+R addr mode */
#define F_DIRECT	0x00000400	/*   direct addressing mode */
#define F_TRAP		0x00000800	/* traping inst */
#define F_LONGLAT	0x00001000	/* long latency inst (for sched) */
#define F_DIRJMP	0x00002000	/* direct jump */
#define F_INDIRJMP	0x00004000	/* indirect jump */
#define F_CALL		0x00008000	/* function call */
#define F_FPCOND	0x00010000	/* FP conditional branch */
#define F_IMM		0x00020000	/* instruction has immediate operand */
#define F_VEC           0x00040000      /* Vector */
#define F_SYNC          0x00080000      /* PPC-style Sync instruction */
#define F_WACILOAD      0x00100000      /* WACI Load */

#endif
