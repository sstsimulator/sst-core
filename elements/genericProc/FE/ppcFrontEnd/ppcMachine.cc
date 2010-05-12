/*
 * powerpc.c - PowerPC definition routines
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sst_stdint.h>
#include <sst_config.h> /* for WORDS_BIGENDIAN */
#if defined(sun) || defined(__sun)
#define __C99FEATURES__
#endif
#include <math.h>

#include "host.h"
#include "misc.h"
#include "ppcMachine.h"
#include "ppcFront.h"
#include "eval.h"
#include "regs.h"
#include "ieee-fp.c"

/* FIXME: currently SimpleScalar/AXP only builds with quadword support... */
#if !defined(HOST_HAS_QWORD)
#error SimpleScalar/PowerPC only builds on hosts with builtin quadword support...
#error Try building with GNU GCC, as it supports quadwords on most machines.
#endif



/* preferred nop instruction definition */
md_inst_t MD_NOP_INST = 0x60000000;/*ori 0,0,0*/

/* opcode mask -> enum md_opcodem, used by decoder (MD_OP_ENUM()) */
enum md_opcode md_mask2op[MD_MAX_MASK+1];
unsigned int md_opoffset[OP_MAX];



/* enum md_opcode -> mask for decoding next level */
unsigned int md_opmask[OP_MAX+1][2] = {
  {0,0}, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) {0,0},
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) {MASK1,MASK2},
#define CONNECT(OP)
#include "powerpc.def"
  {0,0}
};

/* enum md_opcode -> shift for decoding next level */
unsigned int md_opshift[OP_MAX] = {
  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) SHIFT,
#define CONNECT(OP)
#include "powerpc.def"
};

/* enum md_opcode -> description string */
const char *md_op2name[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) NAME,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) NAME,
#define CONNECT(OP)
#include "powerpc.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
const char *md_op2format[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) OPFORM,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) NULL,
#define CONNECT(OP)
#include "powerpc.def"
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
enum md_fu_class md_op2fu[OP_MAX] = {
  FUClass_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) RES,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) FUClass_NA,
#define CONNECT(OP)
#include "powerpc.def"
};



/* enum md_fu_class -> description string */
const char *md_fu2name[NUM_FU_CLASSES] = {
  NULL, /* NA */
  "fu-int-ALU",
  "fu-int-multiply",
  "fu-int-divide",
  "fu-FP-add/sub",
  "fu-FP-comparison",
  "fu-FP-conversion",
  "fu-FP-multiply",
  "fu-FP-divide",
  "fu-FP-sqrt",
  "rd-port",
  "wr-port"
};

/* enum md_opcode -> opcode flags, used by simulators */
unsigned int md_op2flags[OP_MAX] = {
  NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) FLAGS,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) NA,
#define CONNECT(OP)
#include "powerpc.def"
};

const char *md_amode_str[md_amode_NUM] =
{
  "(const)",		/* immediate addressing mode */
  "(gp + const)",	/* global data access through global pointer */
  "(sp + const)",	/* stack access through stack pointer */
  "(fp + const)",	/* stack access through frame pointer */
  "(reg + const)",	/* (reg + const) addressing */
  "(reg + reg)"		/* (reg + reg) addressing */
};

/* symbolic register names, parser is case-insensitive */
md_reg_names_t md_reg_names[] =
{
  /* name */	/* file */	/* reg */

  /* integer register file */
  { "$r0",	rt_gpr,		0 },
  { "$r1",	rt_gpr,		1 },
  { "$sp",	rt_gpr,		1 },
  { "$r2",	rt_gpr,		2 },
  { "$r3",	rt_gpr,		3 },
  { "$r4",	rt_gpr,		4 },
  { "$r5",	rt_gpr,		5 },
  { "$r6",	rt_gpr,		6 },
  { "$r7",	rt_gpr,		7 },
  { "$r8",	rt_gpr,		8 },
  { "$r9",	rt_gpr,		9 },
  { "$r10",	rt_gpr,		10 },
  { "$r11",	rt_gpr,		11 },
  { "$r12",	rt_gpr,		12 },
  { "$r13",	rt_gpr,		13 },
  { "$r14",	rt_gpr,		14 },
  { "$r15",	rt_gpr,		15 },
  { "$r16",	rt_gpr,		16 },
  { "$r17",	rt_gpr,		17 },
  { "$r18",	rt_gpr,		18 },
  { "$r19",	rt_gpr,		19 },
  { "$r20",	rt_gpr,		20 },
  { "$r21",	rt_gpr,		21 },
  { "$r22",	rt_gpr,		22 },
  { "$r23",	rt_gpr,		23 },
  { "$r24",	rt_gpr,		24 },
  { "$r25",	rt_gpr,		25 },
  { "$r26",	rt_gpr,		26 },
  { "$r27",	rt_gpr,		27 },
  { "$r28",	rt_gpr,		28 },
  { "$r29",	rt_gpr,		29 },
  { "$r30",	rt_gpr,		30 },
  { "$r31",	rt_gpr,		31 },
  { "$fp",	rt_gpr,		31 },
  /* floating point register file - double precision */
  { "$f0",	rt_dpr,		0 },
  { "$f1",	rt_dpr,		1 },
  { "$f2",	rt_dpr,		2 },
  { "$f3",	rt_dpr,		3 },
  { "$f4",	rt_dpr,		4 },
  { "$f5",	rt_dpr,		5 },
  { "$f6",	rt_dpr,		6 },
  { "$f7",	rt_dpr,		7 },
  { "$f8",	rt_dpr,		8 },
  { "$f9",	rt_dpr,		9 },
  { "$f10",	rt_dpr,		10 },
  { "$f11",	rt_dpr,		11 },
  { "$f12",	rt_dpr,		12 },
  { "$f13",	rt_dpr,		13 },
  { "$f14",	rt_dpr,		14 },
  { "$f15",	rt_dpr,		15 },
  { "$f16",	rt_dpr,		16 },
  { "$f17",	rt_dpr,		17 },
  { "$f18",	rt_dpr,		18 },
  { "$f19",	rt_dpr,		19 },
  { "$f20",	rt_dpr,		20 },
  { "$f21",	rt_dpr,		21 },
  { "$f22",	rt_dpr,		22 },
  { "$f23",	rt_dpr,		23 },
  { "$f24",	rt_dpr,		24 },
  { "$f25",	rt_dpr,		25 },
  { "$f26",	rt_dpr,		26 },
  { "$f27",	rt_dpr,		27 },
  { "$f28",	rt_dpr,		28 },
  { "$f29",	rt_dpr,		29 },
  { "$f30",	rt_dpr,		30 },
  { "$f31",	rt_dpr,		31 },
  /* miscellaneous registers */
  { "$cr",	rt_ctrl,	0 },
  { "$xer",	rt_ctrl,	1 },
  { "$fpscr",	rt_ctrl,	2 },
  /*Link register*/
  { "$lr",	rt_link,	0 },
  /*Counter register*/
  { "$cntr",	rt_cntr,	0 },
  /* program counters */
  { "$pc",	rt_PC,		0 },
  { "$npc",	rt_NPC,		0 },
  /* sentinel */
  { NULL,	rt_gpr,		0 }
};

/* returns a register name string */
const char *
md_reg_name(enum md_reg_type rt, int reg)
{
  int i;

  for (i=0; md_reg_names[i].str != NULL; i++)
    {
      if (md_reg_names[i].file == rt && md_reg_names[i].reg == reg)
	return md_reg_names[i].str;
    }

  /* not found... */
  return NULL;
}

#if 0
char *						/* err str, NULL for no err */
md_reg_obj(struct ppc_regs_t *regs,	        /* registers to access */
	   int is_write,			/* access type */
	   enum md_reg_type rt,			/* reg bank to probe */
	   int reg,				/* register number */
	   eval_value_t *val)		/* input, output */
{
  switch (rt)
    {
    case rt_gpr:
      if (reg < 0 || reg >= MD_NUM_IREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = regs->regs_R[reg];
	}
      else
	regs->regs_R[reg] = eval_as_uint(*val);
      break;

    case rt_dpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_double;
	  val->value.as_double = regs->regs_F.d[reg];
	}
      else
	regs->regs_F.d[reg] = eval_as_double(*val);
      break;

    case rt_ctrl:
      switch (reg)
	{
	case /* cr */0:
	  if (!is_write)
	    {
	      val->type = et_uint;
	      val->value.as_uint = regs->regs_C.cr;
	    }
	  else
	    regs->regs_C.cr = eval_as_uint(*val);
	  break;

	case /* xer */1:
	  if (!is_write)
	    {
	      val->type = et_uint;
	      val->value.as_uint = regs->regs_C.xer;
	    }
	  else
	    regs->regs_C.xer = eval_as_uint(*val);
	  break;

	case /* FPSCR */2:
	  if (!is_write)
	    {
	      val->type = et_uint;
	      val->value.as_uint = regs->regs_C.fpscr;
	    }
	  else
	    regs->regs_C.fpscr = eval_as_uint(*val);
	  break;

	default:
	  return "register number out of range";
	}
      break;

    case rt_PC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs->regs_PC;
	}
      else
	regs->regs_PC = eval_as_addr(*val);
      break;

    case rt_NPC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs->regs_NPC;
	}
      else
	regs->regs_NPC = eval_as_addr(*val);
      break;

    case rt_link:
      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = regs->regs_L;
	}
      else
	regs->regs_L = eval_as_uint(*val);
      break;

    case rt_cntr:
      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = regs->regs_CNTR;
	}
      else
	regs->regs_CNTR = eval_as_uint(*val);
      break;


    default:
      printf("bogus register bank");
    }

  /* no error */
  return NULL;
}
#endif

/* print integer REG(S) to STREAM */
void
md_print_ireg(md_gpr_t regs, int reg, FILE *stream)
{
  fprintf(stream, "%4s: %12d/0x%08x",
	  md_reg_name(rt_gpr, reg), regs[reg], regs[reg]);
}

void
md_print_iregs(md_gpr_t regs, FILE *stream)
{
  int i;

  for (i=0; i < MD_NUM_IREGS; i += 2)
    {
      md_print_ireg(regs, i, stream);
      fprintf(stream, "  ");
      md_print_ireg(regs, i+1, stream);
      fprintf(stream, "\n");
    }
}

/* print floating point REGS to STREAM */
void
md_print_fpreg(md_fpr_t regs, int reg, FILE *stream)
{
  fprintf(stream, "%4s: %f",
	  md_reg_name(rt_dpr, reg), regs.d[reg]);
}

void
md_print_fpregs(md_fpr_t regs, FILE *stream)
{
  int i;

  /* floating point registers */
  for (i=0; i < MD_NUM_FREGS; i ++)
    {
      md_print_fpreg(regs, i, stream);
      fprintf(stream, "\n");
    }
}

void
md_print_creg(md_ctrl_t regs, int reg, FILE *stream)
{
  /* index is only used to iterate over these registers, hence no enums... */
  switch (reg)
    {
    case 0:
      fprintf(stream, "CR: 0x%08x", regs.cr);
      break;

    case 1:
      fprintf(stream, "XER: 0x%08x", regs.xer);
      break;

    case 2:
      fprintf(stream, "FPSCR: 0x%08x", regs.fpscr);
      break;

    default:
      printf("bogus control register index");
    }
}

void
md_print_cregs(md_ctrl_t regs, FILE *stream)
{
  md_print_creg(regs, 0, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 1, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 2, stream);
  fprintf(stream, "\n");
}



/* intialize the inst decoder, this function builds the ISA decode tables */
void
md_init_decoder(void)
{
  unsigned long max_offset = 0;
  unsigned long offset = 0;
  unsigned int i=0;
  for(i=0;i<(unsigned int)MD_MAX_MASK;i++) md_mask2op[i]=(md_opcode)0;

#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5)\
  if ((MSK)+offset >= MD_MAX_MASK) printf("MASK_MAX is too small");	\
  if (md_mask2op[(MSK)+offset]) {printf("DEFINST: doubly defined opcode %x %x %s %lx. Previous op = %s",OP,MSK, NAME, offset, md_op2name[md_mask2op[(MSK)+offset]]);}\
  md_mask2op[(MSK)+offset]=(OP);md_opoffset[OP]=offset; max_offset=MAX(max_offset,(MSK)+offset);

#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)			\
 if ((MSK)+offset >= MD_MAX_MASK) printf("MASK_MAX is too small");	\
  if (md_mask2op[(MSK)+offset]) printf("DEFLINK: doubly defined opcode %x %x %s %lx. Previous op =%s", OP,MSK,NAME,offset, md_op2name[md_mask2op[(MSK)+offset]]);\
  md_mask2op[(MSK)+offset]=(OP); max_offset=MAX(max_offset,(MSK)+offset); 

#define CONNECT(OP)							\
    offset = max_offset+1; md_opoffset[OP] = offset;

#include "powerpc.def"
}

int carrygenerated(sword_t a, sword_t b)
{
  sword_t c, x, y;
  int i;
  
  if((a==0)||(b==0)) return 0;
  
  c=0;
  for(i=0;i<32;i++){
    
    x = (a>>i)&0x1;
    y = (b>>i)&0x1;
    c = x*y| x*c | y*c;
  }
  return c;
}

int isspNan(qword_t t){
  
qword_t mask=0x0ff0000000000000ll;
 
 if( ((t&mask) == mask) && ((t&0x000fffffffffffffll)!=0) ) return 1;
 
 return 0;
 
}

int isdpNan(qword_t t){
  
qword_t mask=0x7ff0000000000000ll;
 
 if( ((t&mask) == mask) && ((t&0x000fffffffffffffll)!=0) ) return 1;
 
return 0;
 
}

int isdpSNan(qword_t t){
  
  qword_t mask=0x7ff0000000000000ll;
  
  if( ((t&mask) == mask) && ((t&0x000fffffffffffffll)!=0) ){
    
    if ( t & 0x0008000000000000ll) return 0;
    else return 1;
    
  }
  else return 0;
  
}

int isspSNan(qword_t t){
  
  qword_t mask=0x7ff0000000000000ll;
  
  if( ((t&mask) == mask) && ((t&0x000fffffffffffffll)!=0) ){
    
    if ( t & 0x0008000000000000ll) return 0;
    else return 1;
    
  }
  else return 0;
  
} 


int isspQNan(qword_t t){
  
qword_t mask=0x7ff0000000000000ll;
 
 if( ((t&mask) == mask) && ((t&0x000fffffffffffffll)!=0) ){
   
   if ( t & 0x0008000000000000ll) return 1;
   else return 0;
   
 }
 else return 0;
 
}


int isdpQNan(qword_t t){

  qword_t mask=0x7ff0000000000000ll;
  
  if( ((t&mask) == mask) && ((t&0x000fffffffffffffll)!=0) ){         
    
    if ( t & 0x0008000000000000ll) return 1;
    else return 0;
    
  }
  else return 0;
  
}

word_t readLower(const qword_t w) {
  //printf("lower w=%f w=%llx\n", w, w);
  return (word_t)((w) & 0xffffffff);
}

word_t readUpper(const qword_t w) {
  //printf("upper w=%f w=%llx\n", w, w);
  return (word_t)((w>>32) & 0xffffffff);
}

qword_t readWhole(const double d) {
  qword_t w = *((qword_t*)&d);
  //printf("readWhole %f -> %llx\n", d, w);
  return w;
}

qword_t endian_swap(qword_t value)
{
#ifndef WORDS_BIGENDIAN
  qword_t b1 = (value >>  0) & 0xff;
  qword_t b2 = (value >>  8) & 0xff;
  qword_t b3 = (value >> 16) & 0xff;
  qword_t b4 = (value >> 24) & 0xff;
  qword_t b5 = (value >> 32) & 0xff;
  qword_t b6 = (value >> 40) & 0xff;
  qword_t b7 = (value >> 48) & 0xff;
  qword_t b8 = (value >> 56) & 0xff;
  
  qword_t ret =  b1 << 56 | b2 << 48 | b3 << 40 | b4 << 32 |
    b5 << 24 | b6 << 16 | b7 <<  8 | b8 <<  0;
  //printf("%08llx -> %08llx\n", value, ret);
  return ret;
#else
  return value;
#endif
}

dfloat_t convertDWToDouble(qword_t q)
{
  //printf("gets: %llx returns: %f\n", q, *((dfloat_t *)(& q)));
  //return *((dfloat_t *)(& q)); 
  qword_t t = q;
  return *((dfloat_t *)(& t)); 
}


/* disassemble a PowerPC instruction */
void
md_print_insn(md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)		/* output stream */
{
  enum md_opcode op;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_OPCODE(op, inst);
  
  /* disassemble the instruction */
  if (op == OP_PPC_NA || op >= OP_MAX)
    {
      /* bogus instruction */
      fprintf(stream, "<invalid inst: 0x%08x>", inst);
    }
  else
    {
      const char *s;
      
      fprintf(stream, "%-10s", MD_OP_NAME(op));

      s = MD_OP_FORMAT(op);
      unsigned int CurrentInstruction = inst;
      while (*s) {
	switch (*s) {
	case 'a':
	  fprintf(stream, "r%d", RA);
	  break;
	case 'b':
	  fprintf(stream, "r%d", RB);
	  break;
	case 'c':
	  fprintf(stream, "r%d", RC);
	  break;
	case 'd':
	  fprintf(stream, "r%d", RD);
	  break;
	case 'e':
	  fprintf(stream, "r%d", ME);
	  break;
	case 'f':
	  fprintf(stream, "%d", BO);
	  break;
	case 'g':
	  fprintf(stream, "%d", BI);
	  break;
	case 'h':
	  fprintf(stream, "%d", SH);
	  break;
	case 'i':
	  fprintf(stream, "%d", IMM);
	  break;
	case 'j':
	  fprintf(stream, "0x%x", LI);
	  break;
	case 'k':
	  fprintf(stream, "%d", BD);
	  break;
	case 'l':
	  fprintf(stream, "%d", ISSETL);
	  break;
	case 'm':
	  fprintf(stream, "%d", MB);
	  break;
	case 'o':
	  fprintf(stream, "%d", OFS);
	  break;
	case 's':
	  fprintf(stream, "r%d", RS);
	  break;
	case 't':
	  fprintf(stream, "%d", TO);
	case 'u':
	  fprintf(stream, "%u", UIMM);
	  break;
	case 'w':
	  fprintf(stream, "%u", CRFS);
	  break;
	case 'x':
	  fprintf(stream, "%u", CRBD);
	  break;
	case 'y':
	  fprintf(stream, "%u", CRBA);
	  break;
	case 'z':
	  fprintf(stream, "%u", CRBB);
	  break;
	case 'A':
	  fprintf(stream, "r%d", FA);
	  break;
	case 'B':
	  fprintf(stream, "r%d", FB);
	  break;
	case 'C':
	  fprintf(stream, "r%d", FC);
	  break;
	case 'D':
	  fprintf(stream, "f%d", FD);
	  break;
	case 'S':
	  fprintf(stream, "f%d", FS);
	  break;
	case 'N':
	  fprintf(stream, "%d", NB);
	  break;
	case 'M':
	  fprintf(stream, "%d", MTFSFI_FM);
	  break;
	case 'P':
	  fprintf(stream, "%d", SPR);
	  break;
	case 'r':
	  fprintf(stream, "%d", CRFD);
	case 'R':
	  fprintf(stream, "%d", CRM);
	  break;
	case 'U':
	  fprintf(stream, "0x%x", UIMM);
	  break;
	default:
	  /* anything unrecognized, e.g., '.' is just passed through */
	  fputc(*s, stream);
	}
	s++;
      }
    }
}

/*This function takes care of the special extended opcode scenario in instructions with opcode 63*/

unsigned int md_check_mask(md_inst_t inst){

if((MD_TOP_OP(inst))!=63) return 0;

 if(inst&0x20) return 1;/*Use the other opmask of size 6 bits*/

 return 0;/*Use the normal opmask of size 11 bits*/

}



/**********************To be fixed for PowerPC****************************/

/* xor checksum registers */ 
/*Problematic since PowerPC has 64 bit FP registers */

#if 0
word_t
md_xor_regs(struct ppc_regs_t *regs)
{
  int i;
  qword_t checksum = 0;

  for (i=0; i < MD_NUM_IREGS; i++)
    checksum ^= regs->regs_R[i];
   
  for (i=0; i < MD_NUM_FREGS; i++)
    checksum ^= *(qword_t *)(&(regs->regs_F.d[i]));

  checksum ^= regs->regs_C.cr;
  checksum ^= regs->regs_C.xer;
  checksum ^= regs->regs_C.fpscr;
  checksum ^= regs->regs_PC;
  checksum ^= regs->regs_NPC;

  return (word_t)checksum;
}
#endif

unsigned int sim_mask32(unsigned int start, unsigned int end)
{
  return(((start > end) ?
	  ~(((unsigned int)-1 >> start) ^ ((end >= 31) ? 0 : (unsigned int)-1 >> (end + 1)
					   )) : 
	  (((unsigned int)-1 >> start) ^ ((end >= 31) ? 0 : (unsigned int)-1 >> (end+1)))));
}

unsigned int sim_rotate_left_32(unsigned int source, unsigned int count)
{
    unsigned int value;
    
    if (count == 0) 
        return(source);
    
    value = source & sim_mask32(0, count-1);
    value >>= (32 - count);
    value |= (source << count);
    
    return(value);
}


/* wcmclen: following code used in setting FPSCR fields for floating
 *          point operations
 */
#include<math.h>


int FPClassify_D( double value )
{
    long long tmp;
    int status = 0;
    
    memcpy(&tmp, &value, sizeof(tmp));

    if( value == 0.0 )     status |= FLAG_ZERO;
    if( ((tmp>>63)&1)==1)  status |= FLAG_NEG;
    if( isnan(value) )     status |= FLAG_NAN;
    if( isinf(value) )     status |= FLAG_INF;
    if(!isnormal(value))   status |= FLAG_DENORM;
    if( isnan(value) && TEST_SNAN_D(value)) 
      status |= FLAG_SNAN;
    return(status);
}


int FPClassify_S( float value )
{
    unsigned int tmp;
    int status = 0;
    
    memcpy(&tmp, &value, sizeof(tmp));

    if(value == 0.0 )      status |= FLAG_ZERO;
    if( ((tmp>>31)&1)==1)  status |= FLAG_NEG;
    if( isnan(value) )     status |= FLAG_NAN;
    if( isinf(value) )     status |= FLAG_INF;
    if(!isnormal(value))   status |= FLAG_DENORM;
    if( isnan(value) && TEST_SNAN_S(value)) 
      status |= FLAG_SNAN;
    return(status);
}


/* Process rounding flag related fields for FPSCR update */
/* wcm: note, currently there is no way to determine many rounding
 *      cases based on information available in the softfloat to 
 *      exactly match the PPC assembly. 
 */
word_t fpscr_process_rflags(word_t fpscr, int sf_RFlags)
{
  /* Check Rounding flag to see if we need to set FR */
  if(sf_RFlags & 1)
  {
    /* currently not working... can't accurately set rounding in softfloat
     * to sync exactly with PPC spec+experimental results */
      PPCHW_SET_FPSCR_FR(fpscr);
  }
  return(fpscr);
}


/* Process error flag related fields for FPSCR update */
word_t fpscr_process_eflags(word_t fpscr, int sf_EFlags)
{
  /* Set FX bit */
  if(sf_EFlags & 0x1f)
  {
      PPCHW_SET_FPSCR_FX(fpscr);
  }

  /* Set XX bit (also FI for now) (floating point inexact) */
  if(sf_EFlags & float_flag_inexact) 
  {
      PPCHW_SET_FPSCR_XX(fpscr);
      PPCHW_SET_FPSCR_FI(fpscr);
      // PPCHW_SET_FPSCR_FR(fpscr); // need to revisit FR setting
  }

  /* Set ZX Bit if Floating point divide-by-zero occurred */
  if(sf_EFlags & float_flag_divbyzero)
  {
      PPCHW_SET_FPSCR_ZX(fpscr);
      /* on div by zero, FR and FI are cleared. */
      PPCHW_RESET_FPSCR_FR(fpscr);
      PPCHW_RESET_FPSCR_FI(fpscr);
  }
  
  /* Set UX bit if underflow exception occurred */
  if(sf_EFlags & float_flag_underflow) {
      PPCHW_SET_FPSCR_UX(fpscr);
  }

  /* Set OX bit if overflow exception occurred */
  if(sf_EFlags & float_flag_overflow) {
      PPCHW_SET_FPSCR_OX(fpscr);
  }

  /* Set VX bit if invalid operation exception occurred */
  if(sf_EFlags & float_flag_invalid) {
      PPCHW_SET_FPSCR_VX(fpscr);
  }

  /* Set FEX bit (VX && VE) || (OX && OE) || (UX && UE) ... etc.  */
  if((PPCHW_GET_FPSCR_VX(fpscr) & PPCHW_GET_FPSCR_VE(fpscr)) ||
     (PPCHW_GET_FPSCR_OX(fpscr) & PPCHW_GET_FPSCR_OE(fpscr)) ||
     (PPCHW_GET_FPSCR_UX(fpscr) & PPCHW_GET_FPSCR_UE(fpscr)) ||
     (PPCHW_GET_FPSCR_ZX(fpscr) & PPCHW_GET_FPSCR_ZE(fpscr)) ||
     (PPCHW_GET_FPSCR_XX(fpscr) & PPCHW_GET_FPSCR_XE(fpscr))  )
  {
      PPCHW_SET_FPSCR_FEX(fpscr);
  }
  
  return(fpscr);
}


/* Process optype fields for FPSCR update */
int fpscr_process_optype(word_t fpscr,
			 int fp_optype,
			 int fpclass_u,
			 int fpclass_a,
			 int fpclass_b)
{
  /* Set VXSNAN bit if result is signaling NaN 
   *  - SNaN: if highest-order bit of fraction is zero, the NaN is a SNaN
   *  - QNaN: otherwise the NaN is a Quiet NaN.
   *  (sect: 3.3.1.7  pg: 3-21 PPC Doc: G522-0290-01)
   */
  if(fpclass_u & FLAG_SNAN ) {
      PPCHW_SET_FPSCR_VXSNAN(fpscr);
  }
  if ((fpclass_a & FLAG_SNAN) || (fpclass_b & FLAG_SNAN)) {
      /* unless fp_optype == load/store/move/select/mtfsf */
      PPCHW_SET_FPSCR_VXSNAN(fpscr);
  }

  /* Set operation-type specific bits */
  if( (fp_optype & FP_ADDOP)!=0 )
  {
      if( (fpclass_a & (FLAG_INF | FLAG_NEG))==FLAG_INF &&
	  (fpclass_b & (FLAG_INF | FLAG_NEG))==(FLAG_INF|FLAG_NEG) )
      {
          PPCHW_SET_FPSCR_VXISI(fpscr);
      }
  }
  else if( (fp_optype & FP_SUBOP)!=0 )
  {
      if( (fpclass_a & (FLAG_INF | FLAG_NEG))==FLAG_INF &&
	  (fpclass_b & (FLAG_INF | FLAG_NEG))==FLAG_INF )
      {
	  PPCHW_SET_FPSCR_VXISI(fpscr);
      }
  }
  else if( (fp_optype & FP_DIVOP)!=0 )
  {
      if( (fpclass_a & FLAG_INF) && (fpclass_b & FLAG_INF) ) {
          PPCHW_SET_FPSCR_VXIDI(fpscr);
      } else if( (fpclass_a & FLAG_ZERO) && (fpclass_b & FLAG_ZERO) ) {
	  PPCHW_SET_FPSCR_VXZDZ(fpscr);
      }
  }
  else if( (fp_optype & FP_MULOP)!=0 )
  {
    /*wcm: overflow? */
    if( ((fpclass_a & FLAG_INF ) && (fpclass_b & FLAG_ZERO)) ||
	((fpclass_a & FLAG_ZERO) && (fpclass_b & FLAG_INF )) )
      {
        PPCHW_SET_FPSCR_VXIMZ(fpscr);
      }
  }
  else if( (fp_optype & FP_SQRTOP)!=0)
  {
      /* sqrts are invalid if they're of negative non-zero numbers */
      if ((!(fpclass_a & FLAG_ZERO)) && (fpclass_a & FLAG_NEG)) {
	  PPCHW_SET_FPSCR_VXSQRT(fpscr);
      }
  }
  else if( (fp_optype & FP_COMPOP)!=0)
  {
      /* todo: invalid comparison? */
      /* compares are invalid if they involve a NAN */
      if ((fpclass_a & FLAG_NAN) || (fpclass_b & FLAG_NAN)) {
	  PPCHW_SET_FPSCR_VXVC(fpscr);
      }
  }
  else if ((fp_optype & FP_ROUNDOP)!=0)
  {
      /* todo: invalid rounding? */
      /* FR = if rounded up */
      /* FI = if inexact (rely on softfloat) */
      if (fpclass_b & FLAG_INF) {
	  PPCHW_SET_FPSCR_VXCVI(fpscr);
      }
  }
  else
  {
    fprintf(stderr, "Error: unknown fp_optype provided\n");
  }  

  return(fpscr);
}


/* Process FPRF fields for FPSCR update */
int fpscr_process_fprf(word_t fpscr, int fpclass_u)
{
  int fprf = 0;

  /* Set FPRF bits (FPRF = {C, FPCC} 
   * See table 3-10 "Floating-Point Result Flags -- FPSCR[FPRF]
   *   pg. 3-31, sect. 3.3.6 in PPC Doc: G522-0290-01
   */
  if( fpclass_u & FLAG_NAN )
  {
    fprf = 0x11;
  }
  else if( fpclass_u & FLAG_NEG )
  {
      /* negative values */
      if(fpclass_u & FLAG_INF)         fprf = 0x09;
      else if(fpclass_u & FLAG_ZERO)   fprf = 0x12;
      else if(fpclass_u & FLAG_DENORM) fprf = 0x18;
      else                             fprf = 0x08;  /* -normalized */
  } else {
      /* positive values */
      if(fpclass_u & FLAG_INF)         fprf = 0x05;
      else if(fpclass_u & FLAG_ZERO)   fprf = 0x02;
      else if(fpclass_u & FLAG_DENORM) fprf = 0x14;
      else                             fprf = 0x04;  /* +normalized */
  }
  PPCHW_SET_FPSCR_FPRF(fpscr, fprf);
  return(fpscr);
}


/* Update FPSCR for single-precision floating point values */
word_t fpscr_set_S(word_t old_fpscr,    // FPSCR (old)
		   int    fp_optype,    // FP_ADDOP, FP_SUBOP, FP_DIVOP, etc.
		   float  _u,           // result
		   float  _a,           // operand 1
		   float  _b,           // operand 2
		   int    sf_EFlags,    // softfloat_exception_flags
		   int    sf_RFlags)    // softfloat_rounding_flags
{
  word_t new_fpscr;
  int fpclass_u, fpclass_a, fpclass_b;

/* clear non sticky bits in FPSCR */
  new_fpscr = old_fpscr & 0x9ff80fff;  
  
  /* classify operands */
  fpclass_u = FPClassify_S(_u);
  fpclass_a = FPClassify_S(_a);
  fpclass_b = FPClassify_S(_b);

  new_fpscr = fpscr_process_rflags(new_fpscr, sf_RFlags);
  new_fpscr = fpscr_process_eflags(new_fpscr, sf_EFlags);
  new_fpscr = fpscr_process_optype(new_fpscr, fp_optype, 
				   fpclass_u, fpclass_a, 
				   fpclass_b);
  new_fpscr = fpscr_process_fprf(new_fpscr, fpclass_u);
  return(new_fpscr);
}

/* Update FPSCR for double-precision floating point values */
word_t fpscr_set_D(word_t old_fpscr,    // FPSCR (old)
                   int    fp_optype,    // FP_ADDOP, FP_SUBOP, FP_DIVOP, etc.
                   double _u,           // result
                   double _a,           // operand 1
                   double _b,           // operand 2
                   int    sf_EFlags,    // softfloat_exception_flags
		   int    sf_RFlags)    // softfloat_rounding_flags
{
  word_t new_fpscr;
  int fpclass_u, fpclass_a, fpclass_b;

  /* Clear non-sticky bits off old FPSCR */
  new_fpscr = old_fpscr & 0x9ff80fff;

  /* Classify operands (i.e., +/-INF, QNaN, SNaN, etc. */
  fpclass_u = FPClassify_D(_u);
  fpclass_a = FPClassify_D(_a);
  fpclass_b = FPClassify_D(_b);

  new_fpscr = fpscr_process_rflags(new_fpscr, sf_RFlags);
  new_fpscr = fpscr_process_eflags(new_fpscr, sf_EFlags);
  new_fpscr = fpscr_process_optype(new_fpscr, fp_optype, 
				   fpclass_u, fpclass_a, 
				   fpclass_b);
  if (fp_optype != FP_ROUNDOP) {
      new_fpscr = fpscr_process_fprf(new_fpscr, fpclass_u);
  }
  /* Set VX bit if invalid operation exception occurred */
  if (PPCHW_GET_FPSCR_VXSNAN(new_fpscr) || PPCHW_GET_FPSCR_VXISI(new_fpscr) ||
	  PPCHW_GET_FPSCR_VXIDI(new_fpscr) || PPCHW_GET_FPSCR_VXZDZ(new_fpscr) ||
	  PPCHW_GET_FPSCR_VXIMZ(new_fpscr) || PPCHW_GET_FPSCR_VXVC(new_fpscr) ||
	  PPCHW_GET_FPSCR_VXSQRT(new_fpscr) || PPCHW_GET_FPSCR_VXCVI(new_fpscr)) {
      PPCHW_SET_FPSCR_VX(new_fpscr);
  }

  return(new_fpscr);
}


/* Utility function */
void print_fpscr_bits(word_t _fpscr)
{
  printf("\tFPSCR    = %#0x\n", _fpscr  );
  printf("\t :FX     = %x\n", PPCHW_GET_FPSCR_FX(_fpscr));
  printf("\t :FEX    = %x\n", PPCHW_GET_FPSCR_FEX(_fpscr));
  printf("\t :VX     = %x\n", PPCHW_GET_FPSCR_VX(_fpscr));
  printf("\t :OX     = %x\n", PPCHW_GET_FPSCR_OX(_fpscr));
  printf("\t :UX     = %x\n", PPCHW_GET_FPSCR_UX(_fpscr));
  printf("\t :ZX     = %x\n", PPCHW_GET_FPSCR_ZX(_fpscr));
  printf("\t :XX     = %x\n", PPCHW_GET_FPSCR_XX(_fpscr));
  printf("\t :VXSNAN = %x\n", PPCHW_GET_FPSCR_VXSNAN(_fpscr));
  printf("\t :VXISI  = %x\n", PPCHW_GET_FPSCR_VXISI(_fpscr));
  printf("\t :VXIDI  = %x\n", PPCHW_GET_FPSCR_VXIDI(_fpscr));
  printf("\t :VXZDZ  = %x\n", PPCHW_GET_FPSCR_VXZDZ(_fpscr));
  printf("\t :VXIMZ  = %x\n", PPCHW_GET_FPSCR_VXIMZ(_fpscr));
  printf("\t :VXVC   = %x\n", PPCHW_GET_FPSCR_VXVC(_fpscr));
  printf("\t :FR     = %x\n", PPCHW_GET_FPSCR_FR(_fpscr));
  printf("\t :FI     = %x\n", PPCHW_GET_FPSCR_FI(_fpscr));
  printf("\t :FPRF   = %#02x\n", PPCHW_GET_FPSCR_FPRF(_fpscr));
  printf("\t :\tC    = %x\n", PPCHW_GET_FPSCR_FPRF_C(_fpscr));
  printf("\t :\t16   = %x\n", PPCHW_GET_FPSCR_FPRF_16(_fpscr));
  printf("\t :\t17   = %x\n", PPCHW_GET_FPSCR_FPRF_17(_fpscr));
  printf("\t :\t18   = %x\n", PPCHW_GET_FPSCR_FPRF_18(_fpscr));
  printf("\t :\t19   = %x\n", PPCHW_GET_FPSCR_FPRF_19(_fpscr));
  printf("\t :VXSOFT = %x\n", PPCHW_GET_FPSCR_VXSOFT(_fpscr));
  printf("\t :VXSQRT = %x\n", PPCHW_GET_FPSCR_VXSQRT(_fpscr));
  printf("\t :VXCVI  = %x\n", PPCHW_GET_FPSCR_VXCVI(_fpscr));
  printf("\t :RN     = %x\n", PPCHW_GET_FPSCR_RN(_fpscr));
}

// EOF
