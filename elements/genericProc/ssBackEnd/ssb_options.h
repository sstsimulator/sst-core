/*
 * options.h - options package interfaces
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
 * $Id: ssb_options.h,v 1.1.1.1 2006-01-31 16:35:50 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2005/08/16 21:12:55  arodrig6
 * changes for docs
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
 * Revision 1.2  1998/08/27 15:48:03  taustin
 * implemented host interface description in host.h
 *
 * Revision 1.1  1997/03/11  01:31:53  taustin
 * Initial revision
 *
 *
 */

#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * This options package allows the user to specify the name, description,
 * location, and default values of program option variables.  In addition,
 * two builtin options are supported:
 *
 *   -config <filename>		# load options from <filename>
 *   -dumpconfig <filename>	# save current option into <filename>
 *
 * NOTE: all user-installed option names must begin with a `-', e.g., `-debug'
 */

/* option variable classes */
enum opt_class_t {
  oc_int = 0,		/* integer option */
  oc_uint,		/* unsigned integer option */
  oc_float,		/* float option */
  oc_double,		/* double option */
  oc_enum,		/* enumeration option */
  oc_flag,		/* boolean option */
  oc_string,		/* string option */
  oc_NUM
};

//: user-specified option definition 
//!SEC:ssBack
struct opt_opt_t {
  opt_opt_t *next;	/* next option */
  const char *name;			/* option name, e.g., "-foo:bar" */
  const char *desc;			/* option description */
  int nvars;			/* > 1 if var for list options */
  int *nelt;			/* number of elements parsed */
  char *format;			/* option value print format */
  int print;			/* print option during `-dumpconfig'? */
  int accrue;			/* accrue list across uses */
  enum opt_class_t oc;		/* class of this option */
  union opt_variant_t {
    /* oc == oc_int */
    struct opt_for_int_t {
      int *var;			/* pointer to integer option */
    } for_int;
    /* oc == oc_uint */
    struct opt_for_uint_t {
      unsigned int *var;	/* pointer to unsigned integer option */
    } for_uint;
    /* oc == oc_float */
    struct opt_for_float_t {
      float *var;		/* pointer to float option */
    } for_float;
    /* oc == oc_double */
    struct opt_for_double_t {
      double *var;		/* pointer to double option */
    } for_double;
    /* oc == oc_enum, oc_flag */
    struct opt_for_enum_t {
      int *var;			/* ptr to *int* enum option, NOTE: AN INT */
      const char **emap;		/* array of enum strings */
      int *eval;		/* optional array of enum values */
      int emap_sz;		/* number of enum's in arrays */
    } for_enum;
    /* oc == oc_string */
    struct opt_for_string_t {
      char **var;		/* pointer to string pointer option */
    } for_string;
  } variant;
};

/* user-specified argument orphan parser, called when an argument is
   encountered that is not claimed by a user-installed option */
typedef int
(*orphan_fn_t)(int i,		/* index of the orphan'ed argument */
	       int argc,	/* number of program arguments */
	       char **argv);	/* program arguments */

//: option node
//
// an option note, these trail the option list when help or option
// state is printed
//
//!SEC:ssBack
struct opt_note_t {
   opt_note_t *next;	/* next option note */
  char *note;			/* option note */
};

//: option database definition 
//!SEC:ssBack
struct opt_odb_t {
  opt_opt_t *options;	/* user-installed options in option database */
  orphan_fn_t orphan_fn;	/* user-specified orphan parser */
  char *header;			/* options header */
  opt_note_t *notes;	/* option notes */
};

/* create a new option database */
struct opt_odb_t *
opt_new(orphan_fn_t orphan_fn);		/* user-specified orphan parser */

/* free an option database */
void
opt_delete( opt_odb_t *odb);	/* option database */

/* register an integer option variable */
void
opt_reg_int( opt_odb_t *odb,	/* option database */
	    const char *name,			/* option name */
	    const char *desc,			/* option description */
	    int *var,			/* pointer to option variable */
	    int def_val,		/* default value of option variable */
	    int print,			/* print during `-dumpconfig' */
	    const char *format);		/* optional user print format */

/* register an integer option list */
void
opt_reg_int_list( opt_odb_t *odb,	/* option database */
		 const char *name,		/* option name */
		 const char *desc,		/* option description */
		 int *vars,		/* pointer to option array */
		 int nvars,		/* total entries in option array */
		 int *nelt,		/* number of entries parsed */
		 int *def_val,		/* default value of option array */
		 int print,		/* print during `-dumpconfig'? */
		 const char *format,		/* optional user print format */
		 int accrue);		/* accrue list across uses */

/* register an unsigned integer option variable */
void
opt_reg_uint( opt_odb_t *odb,	/* option database */
	     const char *name,		/* option name */
	     const char *desc,		/* option description */
	     unsigned int *var,		/* pointer to option variable */
	     unsigned int def_val,	/* default value of option variable */
	     int print,			/* print during `-dumpconfig'? */
	     const char *format);		/* optional user print format */

/* register an unsigned integer option list */
void
opt_reg_uint_list( opt_odb_t *odb,/* option database */
		  const char *name,		/* option name */
		  const char *desc,		/* option description */
		  unsigned int *vars,	/* pointer to option array */
		  int nvars,		/* total entries in option array */
		  int *nelt,		/* number of elements parsed */
		  unsigned int *def_val,/* default value of option array */
		  int print,		/* print during `-dumpconfig'? */
		  const char *format,		/* optional user print format */
		  int accrue);		/* accrue list across uses */

/* register a single-precision floating point option variable */
void
opt_reg_float( opt_odb_t *odb,	/* option data base */
	      const char *name,		/* option name */
	      const char *desc,		/* option description */
	      float *var,		/* target option variable */
	      float def_val,		/* default variable value */
	      int print,		/* print during `-dumpconfig'? */
	      const char *format);		/* optional value print format */

/* register a single-precision floating point option array */
void
opt_reg_float_list( opt_odb_t *odb,/* option data base */
		   char *name,		/* option name */
		   char *desc,		/* option description */
		   float *vars,		/* target array */
		   int nvars,		/* target array size */
		   int *nelt,		/* number of args parsed goes here */
		   float *def_val,	/* default variable value */
		   int print,		/* print during `-dumpconfig'? */
		   char *format,	/* optional value print format */
		   int accrue);		/* accrue list across uses */

/* register a double-precision floating point option variable */
void
opt_reg_double( opt_odb_t *odb,	/* option data base */
	       char *name,		/* option name */
	       char *desc,		/* option description */
	       double *var,		/* target variable */
	       double def_val,		/* default variable value */
	       int print,		/* print during `-dumpconfig'? */
	       char *format);		/* optional value print format */

/* register a double-precision floating point option array */
void
opt_reg_double_list( opt_odb_t *odb,/* option data base */
		    char *name,		/* option name */
		    char *desc,		/* option description */
		    double *vars,	/* target array */
		    int nvars,		/* target array size */
		    int *nelt,		/* number of args parsed goes here */
		    double *def_val,	/* default variable value */
		    int print,		/* print during `-dumpconfig'? */
		    char *format,	/* optional value print format */
		    int accrue);	/* accrue list across uses */

/* register an enumeration option variable, NOTE: all enumeration option
   variables must be of type `int', since true enum variables may be allocated
   with variable sizes by some compilers */
void
opt_reg_enum( opt_odb_t *odb,	/* option data base */
	     char *name,		/* option name */
	     char *desc,		/* option description */
	     int *var,			/* target variable */
	     char *def_val,		/* default variable value */
	     char **emap,		/* enumeration string map */
	     int *eval,			/* enumeration value map, optional */
	     int emap_sz,		/* size of maps */
	     int print,			/* print during `-dumpconfig'? */
	     char *format);		/* optional value print format */

/* register an enumeration option array, NOTE: all enumeration option variables
   must be of type `int', since true enum variables may be allocated with
   variable sizes by some compilers */
void
opt_reg_enum_list( opt_odb_t *odb,/* option data base */
		  char *name,		/* option name */
		  char *desc,		/* option description */
		  int *vars,		/* target array */
		  int nvars,		/* target array size */
		  int *nelt,		/* number of args parsed goes here */
		  char *def_val,	/* default variable value */
		  char **emap,		/* enumeration string map */
		  int *eval,		/* enumeration value map, optional */
		  int emap_sz,		/* size of maps */
		  int print,		/* print during `-dumpconfig'? */
		  char *format,		/* optional value print format */
		  int accrue);		/* accrue list across uses */

/* register a boolean flag option variable */
void
opt_reg_flag( opt_odb_t *odb,	/* option data base */
	     const char *name,		/* option name */
	     const char *desc,		/* option description */
	     int *var,			/* target variable */
	     int def_val,		/* default variable value */
	     int print,			/* print during `-dumpconfig'? */
	     const char *format);		/* optional value print format */

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
		  int accrue);		/* accrue list across uses */

/* register a string option variable */
void
opt_reg_string( opt_odb_t *odb,	/* option data base */
	       const char *name,		/* option name */
	       const char *desc,		/* option description */
	       char **var,		/* pointer to string option variable */
	       const char *def_val,		/* default variable value */
	       int print,		/* print during `-dumpconfig'? */
	       const char *format);		/* optional value print format */

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
		    int accrue);	/* accrue list across uses */

/* process command line arguments */
void
opt_process_options( opt_odb_t *odb, const char*);

/* print the value of an option */
void
opt_print_option( opt_opt_t *opt,	/* option variable */
		 FILE *fd);		/* output stream */

/* print all options and current values */
void
opt_print_options( opt_odb_t *odb,/* option data base */
		  FILE *fd,		/* output stream */
		  int terse,		/* print terse options? */
		  int notes);		/* include notes? */

/* print option help page with default values */
void
opt_print_help( opt_odb_t *odb,	/* option data base */
	       FILE *fd);		/* output stream */

/* find an option by name in the option database, returns NULL if not found */
 opt_opt_t *
opt_find_option( opt_odb_t *odb,	/* option database */
		char *opt_name);	/* option name */

/* register an options header, the header is printed before the option list */
void
opt_reg_header( opt_odb_t *odb,	/* option database */
	       const char *header);		/* options header string */

/* register an option note, notes are printed after the list of options */
void
opt_reg_note( opt_odb_t *odb,	/* option database */
	     const char *note);		/* option note */

#endif /* OPTIONS_H */
