/* default memory state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_mem_obj( mem_t *mem,		/* memory space to access */
	      int is_write,			/* access type */
	      md_addr_t addr,			/* address to access */
	      char *p,				/* input/output buffer */
	      int nbytes)			/* size of access */
{
  enum mem_cmd cmd;

  if (!is_write)
    cmd = Read;
  else
    cmd = Write;

#if 0
  char *errstr;

  errstr = mem_valid(cmd, addr, nbytes, /* declare */FALSE);
  if (errstr)
    return errstr;
#endif

  /* else, no error, access memory */
  if (spec_mode)
    spec_mem_access(mem, cmd, addr, p, nbytes);
  else
    mem_access(mem, cmd, addr, p, nbytes);

  /* no error */
  return NULL;
}

/* default register state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_reg_obj(regs_t *xregs,		/* registers to access */
	      int is_write,			/* access type */
	      enum md_reg_type rt,		/* reg bank to probe */
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
	  val->value.as_uint = GPR(reg);
	}
      else
	SET_GPR(reg, eval_as_uint(*val));
      break;

    case rt_lpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
	return "register number out of range";

      /* FIXME: this is not portable... */
      abort();
#if 0
      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = FPR_L(reg);
	}
      else
	SET_FPR_L(reg, eval_as_uint(*val));
#endif
      break;

    case rt_fpr:
      /* FIXME: this is not portable... */
      abort();
#if 0
      if (!is_write)
	val->value.as_float = FPR_F(reg);
      else
	SET_FPR_F(reg, val->value.as_float);
#endif
      break;

    case rt_dpr:
      /* FIXME: this is not portable... */
      abort();
#if 0
      /* 1/2 as many regs in this mode */
      if (reg < 0 || reg >= MD_NUM_REGS/2)
	return "register number out of range";

      if (at == at_read)
	val->as_double = FPR_D(reg * 2);
      else
	SET_FPR_D(reg * 2, val->as_double);
#endif
      break;

      /* FIXME: this is not portable... */
#if 0
      abort();
    case rt_hi:
      if (at == at_read)
	val->as_word = HI;
      else
	SET_HI(val->as_word);
      break;
    case rt_lo:
      if (at == at_read)
	val->as_word = LO;
      else
	SET_LO(val->as_word);
      break;
    case rt_FCC:
      if (at == at_read)
	val->as_condition = FCC;
      else
	SET_FCC(val->as_condition);
      break;
#endif

    case rt_PC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs.regs_PC;
	}
      else
	regs.regs_PC = eval_as_addr(*val);
      break;

    case rt_NPC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs.regs_NPC;
	}
      else
	regs.regs_NPC = eval_as_addr(*val);
      break;

    default:
      panic("bogus register bank");
    }

  /* no error */
  return NULL;
}

/* default machine state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		  regs_t *regs,		/* registers to access */
		  mem_t *mem)		/* memory space to access */
{
  if (!cmd || !strcmp(cmd, "help"))
    fprintf(stream,
"mstate commands:\n"
"\n"
"    mstate help   - show all machine-specific commands (this list)\n"
"    mstate stats  - dump all statistical variables\n"
"    mstate res    - dump current functional unit resource states\n"
"    mstate ruu    - dump contents of the register update unit\n"
"    mstate lsq    - dump contents of the load/store queue\n"
"    mstate eventq - dump contents of event queue\n"
"    mstate readyq - dump contents of ready instruction queue\n"
"    mstate cv     - dump contents of the register create vector\n"
"    mstate rspec  - dump contents of speculative regs\n"
"    mstate mspec  - dump contents of speculative memory\n"
"    mstate fetch  - dump contents of fetch stage registers and fetch queue\n"
"\n"
	    );
  else if (!strcmp(cmd, "stats"))
    {
      /* just dump intermediate stats */
      sim_print_stats(stream);
    }
  else if (!strcmp(cmd, "res"))
    {
      /* dump resource state */
      res_dump(fu_pool, stream);
    }
  else if (!strcmp(cmd, "ruu"))
    {
      /* dump RUU contents */
      ruu_dump(stream);
    }
  else if (!strcmp(cmd, "lsq"))
    {
      /* dump LSQ contents */
      lsq_dump(stream);
    }
  else if (!strcmp(cmd, "eventq"))
    {
      /* dump event queue contents */
      eventq_dump(stream);
    }
  else if (!strcmp(cmd, "readyq"))
    {
      /* dump event queue contents */
      readyq_dump(stream);
    }
  else if (!strcmp(cmd, "cv"))
    {
      /* dump event queue contents */
      cv_dump(stream);
    }
  else if (!strcmp(cmd, "rspec"))
    {
      /* dump event queue contents */
      rspec_dump(stream);
    }
  else if (!strcmp(cmd, "mspec"))
    {
      /* dump event queue contents */
      mspec_dump(stream);
    }
  else if (!strcmp(cmd, "fetch"))
    {
      /* dump event queue contents */
      fetch_dump(stream);
    }
  else
    return "unknown mstate command";

  /* no error */
  return NULL;
}
