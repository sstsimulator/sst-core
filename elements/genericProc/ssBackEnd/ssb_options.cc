/*
 * options.c - options package routines
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
 * $Id: ssb_options.cc,v 1.2 2006-10-03 23:08:07 mjleven Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2006/01/31 16:35:50  afrodri
 * first entry
 *
 * Revision 1.6  2005/08/16 21:12:55  arodrig6
 * changes for docs
 *
 * Revision 1.5  2005/06/28 21:21:22  arodrig6
 * changes for GCC 4.0 - mainly making simpleScalar tighter
 *
 * Revision 1.4  2004/11/16 04:17:46  arodrig6
 * added more documentation
 *
 * Revision 1.3  2004/08/17 23:39:04  arodrig6
 * added main and NIC processors
 *
 * Revision 1.2  2004/08/10 22:55:35  arodrig6
 * works, except for the speculative stuff, caches, and multiple procs
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.1.1.1  2000/03/07 05:15:17  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:51  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.3  1998/08/31 17:10:39  taustin
 * ported to MS VC++
 *
 * Revision 1.2  1998/08/27 15:47:46  taustin
 * implemented host interface description in host.h
 * options package fixes: on/off supported for booleans, relative
 *       pathnames, negative integers are now parsed correctly
 *
 * Revision 1.1  1997/03/11  01:31:45  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#else /* _MSC_VER */
#define chdir	_chdir
#define getcwd	_getcwd
#endif
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <errno.h>

//#include "configuration.h"
#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_options.h"

/* create a new option database */
 opt_odb_t *
opt_new(orphan_fn_t orphan_fn)	/* user-specified orphan parser */
{
   opt_odb_t *odb;

  odb = ( opt_odb_t *)calloc(1, sizeof( opt_odb_t));
  if (!odb)
    fatal("out of virtual memory");

  odb->options = NULL;
  odb->orphan_fn = orphan_fn;
  odb->header = NULL;
  odb->notes = NULL;

  return odb;
}

/* free an option database */
void opt_delete( opt_odb_t *odb)	/* option database */
{
   opt_opt_t *opt, *opt_next;
   opt_note_t *note, *note_next;

  /* free all options */
  for (opt=odb->options; opt; opt=opt_next)
    {
      opt_next = opt->next;
      opt->next = NULL;
      free(opt);
    }

  /* free all notes */
  for (note = odb->notes; note != NULL; note = note_next)
    {
      note_next = note->next;
      note->next = NULL;
      free(note);
    }
  odb->notes = NULL;

  free(odb);
}

/* add option OPT to option database ODB */
static void
add_option( opt_odb_t *odb,	/* option database */
	    opt_opt_t *opt)	/* option variable */
{
   opt_opt_t *elt, *prev;

  /* sanity checks on option name */
  if (opt->name[0] != '-')
    panic("option `%s' must start with a `-'", opt->name);

  /* add to end of option list */
  for (prev=NULL, elt=odb->options; elt != NULL; prev=elt, elt=elt->next)
    {
      /* sanity checks on option name */
      if (elt->name[0] == opt->name[0] && !strcmp(elt->name, opt->name))
	panic("option `%s' is multiply defined", opt->name);
    }

  if (prev != NULL)
    prev->next = opt;
  else /* prev == NULL */
    odb->options = opt;
  opt->next = NULL;
}

/* register an integer option variable */
void
opt_reg_int( opt_odb_t *odb,	/* option data base */
	    const char *name,			/* option name */
	    const char *desc,			/* option description */
	    int *var,			/* target variable */
	    int def_val,		/* default variable value */
	    int print,			/* print during `-dumpconfig'? */
	    const char *format)		/* optional value print format */
{
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12d");
  opt->oc = oc_int;
  opt->variant.for_int.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register an integer option list */
void
opt_reg_int_list( opt_odb_t *odb,/* option database */
		 const char *name,		/* option name */
		 const char *desc,		/* option description */
		 int *vars,		/* pointer to option array */
		 int nvars,		/* total entries in option array */
		 int *nelt,		/* number of entries parsed */
		 int *def_val,		/* default value of option array */
		 int print,		/* print during `-dumpconfig'? */
		 const char *format,		/* optional user print format */
		 int accrue)		/* accrue list across uses */
{
  int i;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%d");
  opt->oc = oc_int;
  opt->variant.for_int.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register an unsigned integer option variable */
void
opt_reg_uint( opt_odb_t *odb,	/* option database */
	     const char *name,		/* option name */
	     const char *desc,		/* option description */
	     unsigned int *var,		/* pointer to option variable */
	     unsigned int def_val,	/* default value of option variable */
	     int print,			/* print during `-dumpconfig'? */
	     const char *format)		/* optional user print format */
{
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12u");
  opt->oc = oc_uint;
  opt->variant.for_uint.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register an unsigned integer option list */
void
opt_reg_uint_list( opt_odb_t *odb,/* option database */
		  const char *name,		/* option name */
		  const char *desc,		/* option description */
		  unsigned int *vars,	/* pointer to option array */
		  int nvars,		/* total entries in option array */
		  int *nelt,		/* number of elements parsed */
		  unsigned int *def_val,/* default value of option array */
		  int print,		/* print opt during `-dumpconfig'? */
		  const char *format,		/* optional user print format */
		  int accrue)		/* accrue list across uses */
{
  int i;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%u");
  opt->oc = oc_uint;
  opt->variant.for_uint.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register a single-precision floating point option variable */
void
opt_reg_float( opt_odb_t *odb,	/* option data base */
	      const char *name,		/* option name */
	      const char *desc,		/* option description */
	      float *var,		/* target option variable */
	      float def_val,		/* default variable value */
	      int print,		/* print during `-dumpconfig'? */
	      const char *format)		/* optional value print format */
{
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12.4f");
  opt->oc = oc_float;
  opt->variant.for_float.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a single-precision floating point option array */
void
opt_reg_float_list( opt_odb_t *odb,/* option data base */
		   const char *name,		/* option name */
		   const char *desc,		/* option description */
		   float *vars,		/* target array */
		   int nvars,		/* target array size */
		   int *nelt,		/* number of args parsed goes here */
		   float *def_val,	/* default variable value */
		   int print,		/* print during `-dumpconfig'? */
		   const char *format,	/* optional value print format */
		   int accrue)		/* accrue list across uses */
{
  int i;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%.4f");
  opt->oc = oc_float;
  opt->variant.for_float.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register a double-precision floating point option variable */
void
opt_reg_double( opt_odb_t *odb,	/* option data base */
	       const char *name,		/* option name */
	       const char *desc,		/* option description */
	       double *var,		/* target variable */
	       double def_val,		/* default variable value */
	       int print,		/* print during `-dumpconfig'? */
	       const char *format)		/* option value print format */
{
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12.4f");
  opt->oc = oc_double;
  opt->variant.for_double.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a double-precision floating point option array */
void
opt_reg_double_list( opt_odb_t *odb, /* option data base */
		    const char *name,		/* option name */
		    const char *desc,		/* option description */
		    double *vars,	/* target array */
		    int nvars,		/* target array size */
		    int *nelt,		/* number of args parsed goes here */
		    double *def_val,	/* default variable value */
		    int print,		/* print during `-dumpconfig'? */
		    const char *format,	/* option value print format */
		    int accrue)		/* accrue list across uses */
{
  int i;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%.4f");
  opt->oc = oc_double;
  opt->variant.for_double.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* bind an enumeration string to an enumeration value */
static int
bind_to_enum(char *str,			/* string to bind to an enum */
	     const char **emap,		/* enumeration string map */
	     int *eval,			/* enumeration value map, optional */
	     int emap_sz,		/* size of maps */
	     int *res)			/* enumeration string value result */
{
  int i;

  /* string enumeration string map */
  for (i=0; i<emap_sz; i++)
    {
      if (!strcmp(str, emap[i]))
	{
	  if (eval)
	    {
	      /* bind to eval value */
	      *res = eval[i];
	    }
	  else
	    {
	      /* bind to string index */
	      *res = i;
	    }
	  return TRUE;
	}
    }

  /* not found */
  *res = -1;
  return FALSE;
}

/* bind a enumeration value to an enumeration string */
static const char *
bind_to_str(int val,			/* enumeration value */
	    const char **emap,		/* enumeration string map */
	    int *eval,			/* enumeration value map, optional */
	    int emap_sz)		/* size of maps */
{
  int i;

  if (eval)
    {
      /* bind to first matching eval value */
      for (i=0; i<emap_sz; i++)
	{
	  if (eval[i] == val)
	    {
	      /* found */
	      return emap[i];
	    }
	}
      /* not found */
      return NULL;
    }
  else
    {
      /* bind to string at index */
      if (val >= emap_sz)
	{
	  /* invalid index */
	  return NULL;
	}
      /* else, index is in range */
      return emap[val];
    }
}

/* register an enumeration option variable, NOTE: all enumeration option
   variables must be of type `int', since true enum variables may be allocated
   with variable sizes by some compilers */
void
opt_reg_enum( opt_odb_t *odb,	/* option data base */
	     const char *name,		/* option name */
	     const char *desc,		/* option description */
	     int *var,			/* target variable */
	     char *def_val,		/* default variable value */
	     const char **emap,		/* enumeration string map */
	     int *eval,			/* enumeration value map, optional */
	     int emap_sz,		/* size of maps */
	     int print,			/* print during `-dumpconfig'? */
	     const char *format)		/* option value print format */
{
  int enum_val;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12s");
  opt->oc = oc_enum;
  opt->variant.for_enum.var = var;
  opt->variant.for_enum.emap = emap;
  opt->variant.for_enum.eval = eval;
  opt->variant.for_enum.emap_sz = emap_sz;
  if (def_val)
    {
      if (!bind_to_enum(def_val, emap, eval, emap_sz, &enum_val))
	fatal("could not bind default value for option `%s'", name);
    }
  else
    enum_val = 0;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = enum_val;
}

/* register an enumeration option array, NOTE: all enumeration option variables
   must be of type `int', since true enum variables may be allocated with
   variable sizes by some compilers */
void
opt_reg_enum_list( opt_odb_t *odb,/* option data base */
		  const char *name,		/* option name */
		  const char *desc,		/* option description */
		  int *vars,		/* target array */
		  int nvars,		/* target array size */
		  int *nelt,		/* number of args parsed goes here */
		  char *def_val,	/* default variable value */
		  const char **emap,		/* enumeration string map */
		  int *eval,		/* enumeration value map, optional */
		  int emap_sz,		/* size of maps */
		  int print,		/* print during `-dumpconfig'? */
		  const char *format,		/* option value print format */
		  int accrue)		/* accrue list across uses */
{
  int i, enum_val;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%s");
  opt->oc = oc_enum;
  opt->variant.for_enum.var = vars;
  opt->variant.for_enum.emap = emap;
  opt->variant.for_enum.eval = eval;
  opt->variant.for_enum.emap_sz = emap_sz;
  if (def_val)
    {
      if (!bind_to_enum(def_val, emap, eval, emap_sz, &enum_val))
	fatal("could not bind default value for option `%s'", name);
    }
  else
    enum_val = 0;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = enum_val;
}

/* pre-defined boolean flag operands */
#define NUM_FLAGS		28
static const char *flag_emap[NUM_FLAGS] = {
  "true", "t", "T", "True", "TRUE", "1", "y", "Y", "yes", "Yes", "YES",
  "on", "On", "ON",
  "false", "f", "F", "False", "FALSE", "0", "n", "N", "no", "No", "NO",
  "off", "Off", "OFF"
};
static int flag_eval[NUM_FLAGS] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* register a boolean flag option variable */
void
opt_reg_flag( opt_odb_t *odb,	/* option data base */
	     const char *name,		/* option name */
	     const char *desc,		/* option description */
	     int *var,			/* target variable */
	     int def_val,		/* default variable value */
	     int print,			/* print during `-dumpconfig'? */
	     const char *format)		/* optional value print format */
{
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12s");
  opt->oc = oc_flag;
  opt->variant.for_enum.var = var;
  opt->variant.for_enum.emap = flag_emap;
  opt->variant.for_enum.eval = flag_eval;
  opt->variant.for_enum.emap_sz = NUM_FLAGS;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a boolean flag option array */
void
opt_reg_flag_list( opt_odb_t *odb,/* option database */
		  const char *name,		/* option name */
		  const char *desc,		/* option description */
		  int *vars,		/* pointer to option array */
		  int nvars,		/* total entries in option array */
		  int *nelt,		/* number of elements parsed */
		  int *def_val,		/* default array value */
		  int print,		/* print during `-dumpconfig'? */
		  const char *format,		/* optional value print format */
		  int accrue)		/* accrue list across uses */
{
  int i;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%s");
  opt->oc = oc_flag;
  opt->variant.for_enum.var = vars;
  opt->variant.for_enum.emap = flag_emap;
  opt->variant.for_enum.eval = flag_eval;
  opt->variant.for_enum.emap_sz = NUM_FLAGS;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register a string option variable */
void
opt_reg_string( opt_odb_t *odb,	/* option data base */
	       const char *name,		/* option name */
	       const char *desc,		/* option description */
	       char **var,		/* pointer to string option variable */
	       const char *def_val,		/* default variable value */
	       int print,		/* print during `-dumpconfig'? */
	       const char *format)		/* optional value print format */
{
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = (char*)(format ? format : "%12s");
  opt->oc = oc_string;
  opt->variant.for_string.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = (char *)def_val;
}

/* register a string option array */
void
opt_reg_string_list( opt_odb_t *odb,/* option data base */
		    const char *name,		/* option name */
		    const char *desc,		/* option description */
		    char **vars,	/* pointer to option string array */
		    int nvars,		/* target array size */
		    int *nelt,		/* number of args parsed goes here */
		    char **def_val,	/* default variable value */
		    int print,		/* print during `-dumpconfig'? */
		    const char *format,	/* optional value print format */
		    int accrue)		/* accrue list across uses */
{
  int i;
   opt_opt_t *opt;

  opt = ( opt_opt_t *)calloc(1, sizeof( opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = (char*)(format ? format : "%s");
  opt->oc = oc_string;
  opt->variant.for_string.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* process command line arguments, returns index of next argument to parse */
int
process_option( opt_odb_t *odb,	/* option database */
	       int index,		/* index of the first arg to parse */
	       int argc,		/* total number of arguments */
	       char **argv)		/* argument string array */
{
  int cnt, ent, nvars;
  char *endp;
  double tmp;
   opt_opt_t *opt;

  /* locate the option in the option database */
  for (opt=odb->options; opt != NULL; opt=opt->next)
    {
      if (!strcmp(opt->name, argv[index]))
	break;
    }
  if (!opt)
    {
      /* no one registered this option */
      fatal("option `%s' is undefined", argv[index]);
    }
  index++;

  /* process option arguments */
  switch (opt->oc)
    {
    case oc_int:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !isdigit((int)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || isdigit((int)argv[index][1])))
	{
	  opt->variant.for_int.var[ent] = strtol(argv[index], &endp, 0);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_uint:
      /* this option needs at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  opt->variant.for_uint.var[ent] = strtoul(argv[index], &endp, 0);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_float:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !isdigit((int)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || isdigit((int)argv[index][1])))
	{
	  tmp = strtod(argv[index], &endp);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  if (tmp < FLT_MIN || tmp > FLT_MAX)
	    {
	      /* over/underflow */
	      fatal("FP over/underflow for argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  opt->variant.for_float.var[ent] = (float)tmp;
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_double:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !isdigit((int)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || isdigit((int)argv[index][1])))
	{
	  opt->variant.for_double.var[ent] = strtod(argv[index], &endp);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_enum:
      /* this option needs at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  if (!bind_to_enum(argv[index],
			    opt->variant.for_enum.emap,
			    opt->variant.for_enum.eval,
			    opt->variant.for_enum.emap_sz,
			    &opt->variant.for_enum.var[ent]))
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_flag:
      /* check if this option has at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available, default value for flag is true */
	  opt->variant.for_enum.var[0] = TRUE;
	  break;
	}
      /* else, parse boolean argument(s) */
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  if (!bind_to_enum(argv[index],
			    opt->variant.for_enum.emap,
			    opt->variant.for_enum.eval,
			    opt->variant.for_enum.emap_sz,
			    &opt->variant.for_enum.var[ent]))
	    {
	      /* could not parse entire argument, default to true */
	      opt->variant.for_enum.var[ent] = TRUE;
	      break;
	    }
	  else
	    {
	      /* else, argument converted correctly */
	      if (opt->nelt)
		(*opt->nelt)++;
	      cnt++; index++; ent++;
	    }
	}
      break;
    case oc_string:
      /* this option needs at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  opt->variant.for_string.var[ent] = argv[index];
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    default:
      panic("bogus option class");
    }

  return index;
}

/* forward declarations */
static void process_file( opt_odb_t *odb, const char *fname, int depth);
static void dump_config( opt_odb_t *odb, char *fname);

/* process a command line, internal version that tracks `-config' depth */
static void
__opt_process_options( opt_odb_t *odb,	/* option data base */
		      int argc,			/* number of arguments */
		      char **argv,		/* argument array */
		      int depth)		/* `-config' option depth */
{
  int index, do_dumpconfig;
  char *dumpconfig_name;

  index = 0;
  do_dumpconfig = FALSE;
  dumpconfig_name = NULL;
  /* visit all command line arguments */
  while (index < argc)
    {
      /* process any encountered orphans */
      while (index < argc && argv[index][0] != '-')
	{
	  if (depth > 0)
	    {
	      /* orphans are not allowed during config file processing */
	      fatal("orphan `%s' encountered during config file processing",
		    argv[index]);
	    }
	  /* else, call the user-stalled orphan handler */
	  if (odb->orphan_fn)
	    {
	      if (!odb->orphan_fn(index+1, argc, argv))
		{
		  /* done processing command line */
		  goto done_processing;
		}
	    }
	  else
	    {
	      /* no one claimed this option */
	      fatal("orphan argument `%s' was unclaimed", argv[index]);
	    }
	  /* go to next option */
	}

      /* done with command line? */
      if (index == argc)
	{
	  /* done processing command line */
	  goto done_processing;
	}
      /* when finished, argv[index] is an option to parse */

      /* process builtin options */
      if (!strcmp(argv[index], "-config"))
	{
	  /* handle `-config' builtin option */
	  index++;
	  if (index >= argc || argv[index][0] == '-')
	    {
	      /* no arguments available */
	      fatal("option `-config' requires an argument");
	    }
	  process_file(odb, argv[index], depth);
	  index++;
	}
      else if (!strcmp(argv[index], "-dumpconfig"))
	{
	  /* this is performed *last* */
	  do_dumpconfig = TRUE;
	  /* handle `-dumpconfig' builtin option */
	  index++;
	  if (index >= argc
	      || (argv[index][0] == '-' && argv[index][1] != '\0'))
	    {
	      /* no arguments available */
	      fatal("option `-dumpconfig' requires an argument");
	    }
	  dumpconfig_name = argv[index];
	  index++;
	}
      else
	{
	  /* process user-installed option */
	  index = process_option(odb, index, argc, argv);
	}
    }
 done_processing:

  if (do_dumpconfig)
    dump_config(odb, dumpconfig_name);
}

/* process command line arguments */
void
opt_process_options( opt_odb_t *odb, const char *pName)
{
  process_file(odb, 
	       pName,
	       0);
}

/* handle `-config' builtin option */
#define MAX_LINE_ARGS		256
#define MAX_FILENAME_LEN	1024
static void
process_file( opt_odb_t *odb, const char *fname, int depth)
{
  int largc;
  char *largv[MAX_LINE_ARGS];
  char line[1024], *p, *q;
  char cwd[MAX_FILENAME_LEN];
  char *header;
  char *local_fname = strdup(fname);
  FILE *fd;

  fd = fopen(fname, "r");
  if (!fd)
    fatal("could not open configuration file `%s'", fname);

  if (!getcwd(cwd, MAX_FILENAME_LEN))
    fatal("can't get cwd");

  header = strrchr(local_fname, '/');
  if (header != NULL)
    {
      /* filename is path - get header */
      *header = '\0';
      if (chdir(local_fname) == -1)
	fatal("can't chdir to %s\n", local_fname);
      *header = '/';
    }

  do {
#if 0
      fprintf(stderr, "!feof(fd): %d, strlen(line): %d, line: %s\n",
	      !feof(fd), strlen(line), line);
#endif

      line[0] = '\n';
      /* read a line from the file and chop */
      if (fgets(line, 1024, fd) == NULL) {
	if (ferror(fd)) {
	    fatal("%s generated an error: %s\n", fname, strerror(errno));
	} else {
	    /* eof */
	    break;
	}
      }
      if (line[0] == '\0' || line[0] == '\n')
	continue;
      if (line[strlen(line)-1] == '\n')
	line[strlen(line)-1] = '\0';

      /* process one line from the file */
      largc = 0; p = line;
      while (*p)
	{
	  /* skip whitespace */
	  while (*p != '\0' && (*p == ' ' || *p == '\t'))
	    p++;

	  /* ignore empty lines and comments */
	  if (*p == '\0' || *p == '#')
	    break;

	  /* skip to the end of the argument */
	  q = p;
	  while (*q != '\0' && *q != ' ' && *q != '\t')
	    q++;
	  if (*q)
	    {
	      *q = '\0';
	      q++;
	    }

	  /* marshall an option array */
	  largv[largc++] = mystrdup(p);

	  if (largc == MAX_LINE_ARGS)
	    {
	      if (chdir(cwd) == -1)
		fatal("can't chdir back to %s\n", cwd);
	      fatal("option line too complex in file `%s'", fname);
	    }

	  /* go to next argument */
	  p = q;
	}

      /* process the line */
      if (largc > 0)
	__opt_process_options(odb, largc, largv, depth+1);
      /* else, empty line */
    } while (!feof(fd));

  fclose(fd);
  free(local_fname);

  if (chdir(cwd) == -1)
    fatal("can't chdir back to %s\n", cwd);
}

/* print the value of an option */
void
opt_print_option( opt_opt_t *opt,/* option variable */
		 FILE *fd)		/* output stream */
{
  int i, nelt;

  switch (opt->oc)
    {
    case oc_int:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_int.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_uint:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_uint.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_float:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, (double)opt->variant.for_float.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_double:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_double.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_enum:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  const char *estr = bind_to_str(opt->variant.for_enum.var[i],
				   opt->variant.for_enum.emap,
				   opt->variant.for_enum.eval,
				   opt->variant.for_enum.emap_sz);
	  if (!estr)
	    panic("could not bind enum `%d' for option `%s'",
		  opt->variant.for_enum.var[i], opt->name);

	  fprintf(fd, opt->format, estr);
	  fprintf(fd, " ");
	}
      break;
    case oc_flag:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  const char *estr = bind_to_str(opt->variant.for_enum.var[i],
				   opt->variant.for_enum.emap,
				   opt->variant.for_enum.eval,
				   opt->variant.for_enum.emap_sz);
	  if (!estr)
	    panic("could not bind boolean `%d' for option `%s'",
		  opt->variant.for_enum.var[i], opt->name);
	  
	  fprintf(fd, opt->format, estr);
	  fprintf(fd, " ");
	}
      break;
    case oc_string:
      if (!opt->nvars)
	{
	  fprintf(fd, "%12s ", "<null>");
	  break;
	}
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      if (nelt == 0)
	{
	  fprintf(fd, "%12s ", "<null>");
	  break;
	}
      else
	{
	  for (i=0; i<nelt; i++)
	    {
	      fprintf(fd, opt->format,
		      (opt->variant.for_string.var[i]
		       ? opt->variant.for_string.var[i]
		       : "<null>"));
	      fprintf(fd, " ");
	    }
	}
      break;
    default:
      panic("bogus option class");
    }
}

/* print any options header */
static void
print_option_header( opt_odb_t *odb,/* options database */
		    FILE *fd)		/* output stream */
{
  if (!odb->header)
    return;

  fprintf(fd, "\n%s\n", odb->header);
}

/* print option notes */
static void
print_option_notes( opt_odb_t *odb,/* options database */
		   FILE *fd)		/* output stream */
{
   opt_note_t *note;

  if (!odb->notes)
    return;

  fprintf(fd, "\n");
  for (note=odb->notes; note != NULL; note=note->next)
    fprintf(fd, "%s\n", note->note);
  fprintf(fd, "\n");
}

/* builtin options */
static  opt_opt_t dumpconfig_opt =
  { NULL, "-dumpconfig", "dump configuration to a file",
    0, NULL, NULL, FALSE, FALSE, oc_string };
static  opt_opt_t config_opt =
  { &dumpconfig_opt, "-config", "load configuration from a file",
    0, NULL, NULL, FALSE, FALSE, oc_string };
static  opt_opt_t *builtin_options = &config_opt;

/* return non-zero if the option is a NULL-valued string option */
int					/* non-zero if null string option */
opt_null_string( opt_opt_t *opt)
{
  return (opt != NULL
	  && opt->oc == oc_string
	  && (opt->nvars == 0
	      || (opt->nelt != NULL && *opt->nelt == 0)
	      || (opt->nelt == NULL
		  && (opt->variant.for_string.var == NULL
		      || opt->variant.for_string.var[0] == NULL))));
}

/* print all options and current values */
void
opt_print_options( opt_odb_t *odb,/* option data base */
		  FILE *fd,		/* output stream */
		  int terse,		/* print terse options? */
		  int notes)		/* include notes? */
{
   opt_opt_t *opt;

  if (!odb)
    {
      /* no options */
      return;
    }

  /* print any options header */
  if (notes)
    print_option_header(odb, fd);

  /* dump out builtin options */
  for (opt=builtin_options; opt != NULL; opt=opt->next)
    {
      if (terse)
	fprintf(fd, "# %-27s # %s\n", opt->name, opt->desc);
      else
	{
	  fprintf(fd, "# %s\n", opt->desc);
	  fprintf(fd, "# %-22s\n\n", opt->name);
	}
    }

  /* dump out options from options database */
  for (opt=odb->options; opt != NULL; opt=opt->next)
    {
      if (terse)
	{
	  if (!opt->print || opt_null_string(opt))
	      fprintf(fd, "# %-14s ", opt->name);
	  else
	    fprintf(fd, "%-16s ", opt->name);
	  opt_print_option(opt, fd);
	  if (opt->desc)
	    fprintf(fd, "# %-22s", opt->desc);
	  fprintf(fd, "\n");
	}
      else
	{
	  if (opt->desc)
	    fprintf(fd, "# %s\n", opt->desc);
	  if (!opt->print || opt_null_string(opt))
	    fprintf(fd, "# %-20s ", opt->name);
	  else
	    fprintf(fd, "%-22s ", opt->name);
	  opt_print_option(opt, fd);
	  fprintf(fd, "\n\n");
	}
    }

  /* print option notes */
  if (notes)
    print_option_notes(odb, fd);
}

/* print help information for an option */
static void
print_help( opt_opt_t *opt,	/* option variable */
	   FILE *fd)			/* output stream */
{
  const char *s = NULL;

  fprintf(fd, "%-16s ", opt->name);
  switch (opt->oc)
    {
    case oc_int:
      if (opt->nvars == 1)
	s = "<int>";
      else
	s = "<int list...>";
      break;
    case oc_uint:
      if (opt->nvars == 1)
	s = "<uint>";
      else
	s = "<uint list...>";
      break;
    case oc_float:
      if (opt->nvars == 1)
	s = "<float>";
      else
	s = "<float list...>";
      break;
    case oc_double:
      if (opt->nvars == 1)
	s = "<double>";
      else
	s = "<double list...>";
      break;
    case oc_enum:
      if (opt->nvars == 1)
	s = "<enum>";
      else
	s = "<enum list...>";
      break;
    case oc_flag:
      if (opt->nvars == 1)
	s = "<true|false>";
      else
	s = "<true|false list...>";
      break;
    case oc_string:
      if (opt->nvars == 0 || opt->nvars == 1)
	s = "<string>";
      else
	s = "<string list...>";
      break;
    default:
      panic("bogus option class");
    }
  fprintf(fd, "%-16s # ", s);
  opt_print_option(opt, fd);
  fprintf(fd, "# %-22s\n", opt->desc);
}

/* print option help page with default values */
void
opt_print_help( opt_odb_t *odb,	/* option data base */
	       FILE *fd)		/* output stream */
{
   opt_opt_t *opt;

  /* print any options header */
  print_option_header(odb, fd);

  fprintf(fd, "#\n");
  fprintf(fd, "%-16s %-16s # %12s # %-22s\n",
	  "# -option", "<args>", "<default>", "description");
  fprintf(fd, "#\n");

  /* print out help info for builtin options */
  for (opt=builtin_options; opt != NULL; opt=opt->next)
    print_help(opt, fd);

  /* print out help info for options in options database */
  for (opt=odb->options; opt != NULL; opt=opt->next)
    print_help(opt, fd);

  /* print option notes */
  print_option_notes(odb, fd);
}

/* handle `-dumpconfig' builtin option, print options from file argument */
static void
dump_config( opt_odb_t *odb,	/* option data base */
	    char *fname)		/* output file name, "-" == stdout */
{
  FILE *fd;

  /* open output file stream */
  if (!strcmp(fname, "-"))
    fd = stderr;
  else
    {
      fd = fopen(fname, "w");
      if (!fd)
	fatal("could not open file `%s'", fname);
    }

  /* print current option values to output stream */
  opt_print_options(odb, fd, /* long */FALSE, /* !notes */FALSE);

  /* close output stream, if not a standard stream */
  if (fd != stdout && fd != stderr)
    fclose(fd);
}

/* find an option by name in the option database, returns NULL if not found */
 opt_opt_t *
opt_find_option( opt_odb_t *odb,	/* option database */
		char *opt_name)	/* option name */
{
   opt_opt_t *opt;

  /* search builtin options */
  for (opt = builtin_options; opt != NULL; opt = opt->next)
    {
      if (!strcmp(opt->name, opt_name))
	{
	  /* located option */
	  return opt;
	}
    }

  /* search user-installed options */
  for (opt = odb->options; opt != NULL; opt = opt->next)
    {
      if (!strcmp(opt->name, opt_name))
	{
	  /* located option */
	  return opt;
	}
    }
  /* not found */
  return NULL;
}

/* register an options header, the header is printed before the option list */
void
opt_reg_header( opt_odb_t *odb,	/* option database */
	       const char *header)		/* options header string */
{
  odb->header = (char *)header;
}

/* register an option note, notes are printed after the list of options */
void
opt_reg_note( opt_odb_t *odb,	/* option database */
	     const char *note_str)		/* option note */
{
   opt_note_t *note, *elt, *prev;

  note = ( opt_note_t *)calloc(1, sizeof( opt_note_t));
  if (!note)
    fatal("out of virtual memory");

  /* record note */
  note->next = NULL;
  note->note = (char *)note_str;

  /* add to end of option notes list */
  for (prev=NULL, elt=odb->notes; elt != NULL; prev=elt, elt=elt->next)
    /* nada */;

  if (prev != NULL)
    prev->next = note;
  else /* prev == NULL */
    odb->notes = note;
  note->next = NULL;
}


#ifdef TEST

int
f_orphan_fn(int i, int argc, char **argv)
{
  fprintf(stdout, "orphans detected at index %d, arg = `%s'...\n", i, argv[i]);

  /* done processing */
  return FALSE;
}

#define MAX_VARS 4
void
main(int argc, char **argv)
{
   opt_odb_t *odb;

  int n_int_vars, n_uint_vars, n_float_vars, n_double_vars;
  int n_enum_vars, n_enum_eval_vars, n_flag_vars, n_string_vars;
  int int_var, int_vars[MAX_VARS];
  unsigned int uint_var, uint_vars[MAX_VARS];
  float float_var, float_vars[MAX_VARS];
  double double_var, double_vars[MAX_VARS];
  int flag_var, flag_vars[MAX_VARS];
  char *string_var, *string_vars[MAX_VARS];

  enum etest_t { enum_a, enum_b, enum_c, enum_d, enum_NUM };
  static char *enum_emap[enum_NUM] =
    { "enum_a", "enum_b", "enum_c", "enum_d" };
  static int enum_eval[enum_NUM] =
    { enum_d, enum_c, enum_b, enum_a };
  int enum_var, enum_vars[MAX_VARS];
  int enum_eval_var, enum_eval_vars[MAX_VARS];

  /* get an options processor */
  odb = opt_new(f_orphan_fn);


  opt_reg_int(odb, "-opt:int", "This is an integer option.",
	      &int_var, 1, /* print */TRUE, /* default format */NULL);
  opt_reg_int_list(odb, "-opt:int:list", "This is an integer list option.",
		   int_vars, MAX_VARS, &n_int_vars, 2,
		   /* print */TRUE, /* default format */NULL);
  opt_reg_uint(odb, "-opt:uint", "This is an unsigned integer option.",
	       &uint_var, 3, /* print */TRUE, /* default format */NULL);
  opt_reg_uint_list(odb, "-opt:uint:list",
		    "This is an unsigned integer list option.",
		    uint_vars, MAX_VARS, &n_uint_vars, 4,
		    /* print */TRUE, /* default format */NULL);
  opt_reg_float(odb, "-opt:float", "This is a float option.",
		&float_var, 5.0, /* print */TRUE, /* default format */NULL);
  opt_reg_float_list(odb, "-opt:float:list", "This is a float list option.",
		     float_vars, MAX_VARS, &n_float_vars, 6.0,
		     /* print */TRUE, /* default format */NULL);
  opt_reg_double(odb, "-opt:double", "This is a double option.",
		 &double_var, 7.0, /* print */TRUE, /* default format */NULL);
  opt_reg_double_list(odb, "-opt:double:list", "This is a double list option.",
		      double_vars, MAX_VARS, &n_double_vars, 8.0,
		      /* print */TRUE, /* default format */NULL);
  opt_reg_enum(odb, "-opt:enum", "This is an enumeration option.",
	       &enum_var, "enum_a", enum_emap, /* index map */NULL, enum_NUM,
	       /* print */TRUE, /* default format */NULL);
  opt_reg_enum_list(odb, "-opt:enum:list", "This is a enum list option.",
		    enum_vars, MAX_VARS, &n_enum_vars, "enum_b",
		    enum_emap, /* index map */NULL, enum_NUM,
		    /* print */TRUE, /* default format */NULL);
  opt_reg_enum(odb, "-opt:enum:eval", "This is an enumeration option w/eval.",
	       &enum_eval_var, "enum_b", enum_emap, enum_eval, enum_NUM,
	       /* print */TRUE, /* default format */NULL);
  opt_reg_enum_list(odb, "-opt:enum:eval:list",
		    "This is a enum list option w/eval.",
		    enum_eval_vars, MAX_VARS, &n_enum_eval_vars, "enum_a",
		    enum_emap, enum_eval, enum_NUM,
		    /* print */TRUE, /* default format */NULL);
  opt_reg_flag(odb, "-opt:flag", "This is a boolean flag option.",
	       &flag_var, FALSE, /* print */TRUE, /* default format */NULL);
  opt_reg_flag_list(odb, "-opt:flag:list",
		    "This is a boolean flag list option.",
		    flag_vars, MAX_VARS, &n_flag_vars, TRUE,
		    /* print */TRUE, /* default format */NULL);
  opt_reg_string(odb, "-opt:string", "This is a string option.",
		 &string_var, "a:string",
		 /* print */TRUE, /* default format */NULL);
  opt_reg_string_list(odb, "-opt:string:list", "This is a string list option.",
		    string_vars, MAX_VARS, &n_string_vars, "another:string",
		    /* print */TRUE, /* default format */NULL);

  /* parse options */
  opt_process_options(odb, argc, argv);

  /* print options */
  fprintf(stdout, "## Current Option Values ##\n");
  opt_print_options(odb, stdout, /* long */FALSE, /* notes */TRUE);

  /* all done */
  opt_delete(odb);
  exit(0);
}

#endif /* TEST */
