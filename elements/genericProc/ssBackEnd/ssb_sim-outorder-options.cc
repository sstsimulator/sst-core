#include "ssb_sim-outorder.h"
#include "ssb_resource.h"
#include "ssb_fu_config-defs.h"
#include "ssb_cache.h"

extern struct res_desc fu_config[];

//: register config options 
void convProc::sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-outorder: This simulator implements a very detailed out-of-order issue\n"
"superscalar processor with a two-level memory system and speculative\n"
"execution support.  This simulator is a performance simulator, tracking the\n"
"latency of all pipeline operations.\n"
		 );

  /* instruction limit */

  opt_reg_uint(odb, "-max:inst", "maximum number of insts to simulate",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  /* trace options */

  opt_reg_int(odb, "-fastfwd", "number of insts to execute in fast fwd mode before timing simulation starts",
	      &fastfwd_count, /* default */0,
	      /* print */TRUE, /* format */NULL);
  
  opt_reg_int(odb, "-stop", "run sim-outorder in fast fwd mode until this PC",
	      (int*)&stop_PC, /* default */ 0, TRUE, NULL);

  /* stdio redirection options */

  opt_reg_string_list(odb, "-ptrace",
	      "generate pipetrace, i.e., <fname|stdout|stderr> <range>",
	      ptrace_opts, /* arr_sz */2, &ptrace_nelt, /* default */NULL,
	      /* !print */FALSE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_note(odb,
"  Pipetrace range arguments are formatted as follows:\n"
"\n"
"    {{@|#}<start>}:{{@|#|+}<end>}\n"
"\n"
"  Both ends of the range are optional, if neither are specified, the entire\n"
"  execution is traced.  Ranges that start with a `@' designate an address\n"
"  range to be traced, those that start with an `#' designate a cycle count\n"
"  range.  All other range values represent an instruction count range.  The\n"
"  second argument, if specified with a `+', indicates a value relative\n"
"  to the first argument, e.g., 1000:+100 == 1000:1100.  Program symbols may\n"
"  be used in all contexts.\n"
"\n"
"    Examples:   -ptrace FOO.trc #0:#1000\n"
"                -ptrace BAR.trc @2000:\n"
"                -ptrace BLAH.trc :1500\n"
"                -ptrace UXXE.trc :\n"
"                -ptrace FOOBAR.trc @main:+278\n"
	       );

  /* ifetch options */

  opt_reg_int(odb, "-fetch:ifqsize", "instruction fetch queue size (in insts)",
	      &ruu_ifq_size, /* default */4,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-fetch:mplat", "extra branch mis-prediction latency",
	      &ruu_branch_penalty, /* default */3,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-fetch:speed",
	      "speed of front-end of machine relative to execution core",
	      &fetch_speed, /* default */1,
	      /* print */TRUE, /* format */NULL);

  /* branch predictor options */

  opt_reg_note(odb,
"  Branch predictor configuration examples for 2-level predictor:\n"
"    Configurations:   N, M, W, X\n"
"      N   # entries in first level (# of shift register(s))\n"
"      W   width of shift register(s)\n"
"      M   # entries in 2nd level (# of counters, or other FSM)\n"
"      X   (yes-1/no-0) xor history and address for 2nd level index\n"
"    Sample predictors:\n"
"      GAg     : 1, W, 2^W, 0\n"
"      GAp     : 1, W, M (M > 2^W), 0\n"
"      PAg     : N, W, 2^W, 0\n"
"      PAp     : N, W, M (M == 2^(N+W)), 0\n"
"      gshare  : 1, W, 2^W, 1\n"
"  Predictor `comb' combines a bimodal and a 2-level predictor.\n"
               );

  opt_reg_string(odb, "-bpred",
		 "branch predictor type {nottaken|taken|perfect|bimod|2lev|comb}",
                 &pred_type, /* default */"bimod",
                 /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:bimod",
		   "bimodal predictor config (<table size>)",
		   bimod_config, bimod_nelt, &bimod_nelt,
		   /* default */bimod_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:2lev",
                   "2-level predictor config "
		   "(<l1size> <l2size> <hist_size> <xor>)",
                   twolev_config, twolev_nelt, &twolev_nelt,
		   /* default */twolev_config,
                   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:comb",
		   "combining predictor config (<meta_table_size>)",
		   comb_config, comb_nelt, &comb_nelt,
		   /* default */comb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int(odb, "-bpred:ras",
              "return address stack size (0 for no return stack)",
              &ras_size, /* default */ras_size,
              /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:btb",
		   "BTB config (<num_sets> <associativity>)",
		   btb_config, btb_nelt, &btb_nelt,
		   /* default */btb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_string(odb, "-bpred:spec_update",
		 "speculative predictors update in {ID|WB} (default non-spec)",
		 &bpred_spec_opt, /* default */NULL,
		 /* print */TRUE, /* format */NULL);

  /* decode options */

  opt_reg_int(odb, "-decode:width",
	      "instruction decode B/W (insts/cycle)",
	      &ruu_decode_width, /* default */4,
	      /* print */TRUE, /* format */NULL);

  /* issue options */

  opt_reg_int(odb, "-issue:width",
	      "instruction issue B/W (insts/cycle)",
	      &ruu_issue_width, /* default */4,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-issue:inorder", "run pipeline with in-order issue",
	       &ruu_inorder_issue, /* default */FALSE,
	       /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-issue:wrongpath",
	       "issue instructions down wrong execution paths",
	       &ruu_include_spec, /* default */TRUE,
	       /* print */TRUE, /* format */NULL);

  /* commit options */

  opt_reg_int(odb, "-commit:width",
	      "instruction commit B/W (insts/cycle)",
	      &ruu_commit_width, /* default */4,
	      /* print */TRUE, /* format */NULL);

  /* register scheduler options */

  opt_reg_int(odb, "-ruu:size",
	      "register update unit (RUU) size",
	      &RUU_size, /* default */16,
	      /* print */TRUE, /* format */NULL);

  /* memory scheduler options  */

  opt_reg_int(odb, "-lsq:size",
	      "load/store queue (LSQ) size",
	      &LSQ_size, /* default */8,
	      /* print */TRUE, /* format */NULL);

  /* cache options */

  opt_reg_string(odb, "-cache:dl1",
		 "l1 data cache config, i.e., {<config>|none}",
		 &cache_dl1_opt, "dl1:128:32:4:l",
		 /* print */TRUE, NULL);

  opt_reg_note(odb,
"  The cache config parameter <config> has the following format:\n"
"\n"
"    <name>:<nsets>:<bsize>:<assoc>:<repl>\n"
"\n"
"    <name>   - name of the cache being defined\n"
"    <nsets>  - number of sets in the cache\n"
"    <bsize>  - block size of the cache\n"
"    <assoc>  - associativity of the cache\n"
"    <repl>   - block replacement strategy, 'l'-LRU, 'f'-FIFO, 'r'-random\n"
"\n"
"    Examples:   -cache:dl1 dl1:4096:32:1:l\n"
"                -dtlb dtlb:128:4096:32:r\n"
	       );

  opt_reg_int(odb, "-cache:dl1lat",
	      "l1 data cache hit latency (in cycles)",
	      &cache_dl1_lat, /* default */1,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:dl2",
		 "l2 data cache config, i.e., {<config>|none}",
		 &cache_dl2_opt, "ul2:1024:64:4:l",
		 /* print */TRUE, NULL);

  opt_reg_int(odb, "-cache:dl2lat",
	      "l2 data cache hit latency (in cycles)",
	      &cache_dl2_lat, /* default */6,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:il1",
		 "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
		 &cache_il1_opt, "il1:512:32:1:l",
		 /* print */TRUE, NULL);

  opt_reg_note(odb,
"  Cache levels can be unified by pointing a level of the instruction cache\n"
"  hierarchy at the data cache hiearchy using the \"dl1\" and \"dl2\" cache\n"
"  configuration arguments.  Most sensible combinations are supported, e.g.,\n"
"\n"
"    A unified l2 cache (il2 is pointed at dl2):\n"
"      -cache:il1 il1:128:64:1:l -cache:il2 dl2\n"
"      -cache:dl1 dl1:256:32:1:l -cache:dl2 ul2:1024:64:2:l\n"
"\n"
"    Or, a fully unified cache hierarchy (il1 pointed at dl1):\n"
"      -cache:il1 dl1\n"
"      -cache:dl1 ul1:256:32:1:l -cache:dl2 ul2:1024:64:2:l\n"
	       );

  opt_reg_int(odb, "-cache:il1lat",
	      "l1 instruction cache hit latency (in cycles)",
	      &cache_il1_lat, /* default */1,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:il2",
		 "l2 instruction cache config, i.e., {<config>|dl2|none}",
		 &cache_il2_opt, "dl2",
		 /* print */TRUE, NULL);

  opt_reg_int(odb, "-cache:il2lat",
	      "l2 instruction cache hit latency (in cycles)",
	      &cache_il2_lat, /* default */6,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-cache:flush", "flush caches on system calls",
	       &flush_on_syscalls, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-cache:icompress",
	       "convert 64-bit inst addresses to 32-bit inst equivalents",
	       &compress_icache_addrs, /* default */FALSE,
	       /* print */TRUE, NULL);

  /* mem options */
  opt_reg_int_list(odb, "-mem:lat",
		   "memory access latency (<first_chunk> <inter_chunk>)",
		   mem_lat, mem_nelt, &mem_nelt, mem_lat,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int(odb, "-mem:width", "memory access bus width (in bytes)",
	      &mem_bus_width, /* default */8,
	      /* print */TRUE, /* format */NULL);

  /* TLB options */

  opt_reg_string(odb, "-tlb:itlb",
		 "instruction TLB config, i.e., {<config>|none}",
		 &itlb_opt, "itlb:16:4096:4:l", /* print */TRUE, NULL);

  opt_reg_string(odb, "-tlb:dtlb",
		 "data TLB config, i.e., {<config>|none}",
		 &dtlb_opt, "dtlb:32:4096:4:l", /* print */TRUE, NULL);

  opt_reg_int(odb, "-tlb:lat",
	      "inst/data TLB miss latency (in cycles)",
	      &tlb_miss_lat, /* default */30,
	      /* print */TRUE, /* format */NULL);

  /* resource configuration */

  opt_reg_int(odb, "-res:ialu",
	      "total number of integer ALU's available",
	      &res_ialu, /* default */fu_config[FU_IALU_INDEX].quantity,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:imult",
	      "total number of integer multiplier/dividers available",
	      &res_imult, /* default */fu_config[FU_IMULT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:memport",
	      "total number of memory system ports available (to CPU)",
	      &res_memport, /* default */fu_config[FU_MEMPORT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fpalu",
	      "total number of floating point ALU's available",
	      &res_fpalu, /* default */fu_config[FU_FPALU_INDEX].quantity,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fpmult",
	      "total number of floating point multiplier/dividers available",
	      &res_fpmult, /* default */fu_config[FU_FPMULT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string_list(odb, "-pcstat",
		      "profile stat(s) against text addr's (mult uses ok)",
		      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);

  //opt_reg_flag(odb, "-bugcompat",
  //       "operate in backward-compatible bugs mode (for testing only)",
  //       &bugcompat_mode, /* default */FALSE, /* print */TRUE, NULL);
}

//: check option values
void convProc::sim_check_options(struct opt_odb_t *odb)
{
  char name[128], c;
  int nsets, bsize, assoc;

  if (fastfwd_count < 0 || fastfwd_count >= 2147483647)
    fatal("bad fast forward count: %d", fastfwd_count);

  if (ruu_ifq_size < 1 || (ruu_ifq_size & (ruu_ifq_size - 1)) != 0)
    fatal("inst fetch queue size must be positive > 0 and a power of two");

  if (ruu_branch_penalty < 1)
    fatal("mis-prediction penalty must be at least 1 cycle");

  if (fetch_speed < 1)
    fatal("front-end speed must be positive and non-zero");

  if (!mystricmp(pred_type, "perfect"))
    {
      /* perfect predictor */
      pred = NULL;
      pred_perfect = TRUE;
    }
  else if (!mystricmp(pred_type, "taken"))
    {
      /* static predictor, not taken */
      pred = bpred_create(BPredTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  else if (!mystricmp(pred_type, "nottaken"))
    {
      /* static predictor, taken */
      pred = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  else if (!mystricmp(pred_type, "bimod"))
    {
      /* bimodal predictor, bpred_create() checks BTB_SIZE */
      if (bimod_nelt != 1)
	fatal("bad bimod predictor config (<table_size>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      /* bimodal predictor, bpred_create() checks BTB_SIZE */
      pred = bpred_create(BPred2bit,
			  /* bimod table size */bimod_config[0],
			  /* 2lev l1 size */0,
			  /* 2lev l2 size */0,
			  /* meta table size */0,
			  /* history reg size */0,
			  /* history xor address */0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else if (!mystricmp(pred_type, "2lev"))
    {
      /* 2-level adaptive predictor, bpred_create() checks args */
      if (twolev_nelt != 4)
	fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      pred = bpred_create(BPred2Level,
			  /* bimod table size */0,
			  /* 2lev l1 size */twolev_config[0],
			  /* 2lev l2 size */twolev_config[1],
			  /* meta table size */0,
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else if (!mystricmp(pred_type, "comb"))
    {
      /* combining predictor, bpred_create() checks args */
      if (twolev_nelt != 4)
	fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)");
      if (bimod_nelt != 1)
	fatal("bad bimod predictor config (<table_size>)");
      if (comb_nelt != 1)
	fatal("bad combining predictor config (<meta_table_size>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      pred = bpred_create(BPredComb,
			  /* bimod table size */bimod_config[0],
			  /* l1 size */twolev_config[0],
			  /* l2 size */twolev_config[1],
			  /* meta table size */comb_config[0],
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else
    fatal("cannot parse predictor type `%s'", pred_type);

  if (!bpred_spec_opt)
    bpred_spec_update = spec_CT;
  else if (!mystricmp(bpred_spec_opt, "ID"))
    bpred_spec_update = spec_ID;
  else if (!mystricmp(bpred_spec_opt, "WB"))
    bpred_spec_update = spec_WB;
  else
    fatal("bad speculative update stage specifier, use {ID|WB}");

  if (ruu_decode_width < 1 || (ruu_decode_width & (ruu_decode_width-1)) != 0)
    fatal("decode width must be positive non-zero and a power of two");

  if (ruu_issue_width < 1 || (ruu_issue_width & (ruu_issue_width-1)) != 0)
    fatal("issue width must be positive non-zero and a power of two");

  if (ruu_commit_width < 1)
    fatal("commit width must be positive non-zero");

  if (RUU_size < 2 || (RUU_size & (RUU_size-1)) != 0)
    fatal("RUU size must be a positive number > 1 and a power of two");

  if (LSQ_size < 2 || (LSQ_size & (LSQ_size-1)) != 0)
    fatal("LSQ size must be a positive number > 1 and a power of two");

  /* use a level 1 D-cache? */
  if (!mystricmp(cache_dl1_opt, "none"))
    {
      cache_dl1 = NULL;

      /* the level 2 D-cache cannot be defined */
      if (strcmp(cache_dl2_opt, "none"))
	fatal("the l1 data cache must defined if the l2 cache is defined");
      cache_dl2 = NULL;
    }
  else /* dl1 is defined */
    {
      if (sscanf(cache_dl1_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad l1 D-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>");
      cache_dl1 = cache_create(name, this, nsets, bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c),
			       &convProc::dl1_access_fn, /* hit lat */cache_dl1_lat);

      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_dl2_opt, "none"))
	cache_dl2 = NULL;
      else
	{
	  if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%c",
		     name, &nsets, &bsize, &assoc, &c) != 5)
	    fatal("bad l2 D-cache parms: "
		  "<name>:<nsets>:<bsize>:<assoc>:<repl>");
	  cache_dl2 = cache_create(name, this, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   &convProc::dl2_access_fn, /* hit lat */cache_dl2_lat);
	}
    }

  /* use a level 1 I-cache? */
  if (!mystricmp(cache_il1_opt, "none"))
    {
      cache_il1 = NULL;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else if (!mystricmp(cache_il1_opt, "dl1"))
    {
      if (!cache_dl1)
	fatal("I-cache l1 cannot access D-cache l1 as it's undefined");
      cache_il1 = cache_dl1;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else if (!mystricmp(cache_il1_opt, "dl2"))
    {
      if (!cache_dl2)
	fatal("I-cache l1 cannot access D-cache l2 as it's undefined");
      cache_il1 = cache_dl2;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else /* il1 is defined */
    {
      if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad l1 I-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>");
      cache_il1 = cache_create(name, this, nsets, bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c),
			       &convProc::il1_access_fn, /* hit lat */cache_il1_lat);

      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_il2_opt, "none"))
	cache_il2 = NULL;
      else if (!mystricmp(cache_il2_opt, "dl2"))
	{
	  if (!cache_dl2)
	    fatal("I-cache l2 cannot access D-cache l2 as it's undefined");
	  cache_il2 = cache_dl2;
	}
      else
	{
	  if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%c",
		     name, &nsets, &bsize, &assoc, &c) != 5)
	    fatal("bad l2 I-cache parms: "
		  "<name>:<nsets>:<bsize>:<assoc>:<repl>");
	  cache_il2 = cache_create(name, this, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   &convProc::il2_access_fn, /* hit lat */cache_il2_lat);
	}
    }

  /* use an I-TLB? */
  if (!mystricmp(itlb_opt, "none"))
    itlb = NULL;
  else
    {
      if (sscanf(itlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>");
      itlb = cache_create(name, this, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c), &convProc::itlb_access_fn,
			  /* hit latency */1);
    }

  /* use a D-TLB? */
  if (!mystricmp(dtlb_opt, "none"))
    dtlb = NULL;
  else
    {
      if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>");
      dtlb = cache_create(name, this, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c), &convProc::dtlb_access_fn,
			  /* hit latency */1);
    }

  if (cache_dl1_lat < 1)
    fatal("l1 data cache latency must be greater than zero");

  if (cache_dl2_lat < 1)
    fatal("l2 data cache latency must be greater than zero");

  if (cache_il1_lat < 1)
    fatal("l1 instruction cache latency must be greater than zero");

  if (cache_il2_lat < 1)
    fatal("l2 instruction cache latency must be greater than zero");

  if (mem_nelt != 2)
    fatal("bad memory access latency (<first_chunk> <inter_chunk>)");

  if (mem_lat[0] < 1 || mem_lat[1] < 1)
    fatal("all memory access latencies must be greater than zero");

  if (mem_bus_width < 1 || (mem_bus_width & (mem_bus_width-1)) != 0)
    fatal("memory bus width must be positive non-zero and a power of two");

  if (tlb_miss_lat < 1)
    fatal("TLB miss latency must be greater than zero");

  if (res_ialu < 1)
    fatal("number of integer ALU's must be greater than zero");
  if (res_ialu > MAX_INSTS_PER_CLASS)
    fatal("number of integer ALU's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_IALU_INDEX].quantity = res_ialu;
  
  if (res_imult < 1)
    fatal("number of integer multiplier/dividers must be greater than zero");
  if (res_imult > MAX_INSTS_PER_CLASS)
    fatal("number of integer mult/div's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_IMULT_INDEX].quantity = res_imult;
  
  if (res_memport < 1)
    fatal("number of memory system ports must be greater than zero");
  if (res_memport > MAX_INSTS_PER_CLASS)
    fatal("number of memory system ports must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_MEMPORT_INDEX].quantity = res_memport;
  
  if (res_fpalu < 1)
    fatal("number of floating point ALU's must be greater than zero");
  if (res_fpalu > MAX_INSTS_PER_CLASS)
    fatal("number of floating point ALU's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_FPALU_INDEX].quantity = res_fpalu;
  
  if (res_fpmult < 1)
    fatal("number of floating point multiplier/dividers must be > zero");
  if (res_fpmult > MAX_INSTS_PER_CLASS)
    fatal("number of FP mult/div's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_FPMULT_INDEX].quantity = res_fpmult;
}

//: register statistics 
void convProc::sim_reg_stats(struct stat_sdb_t *sdb)   /* stats database */
{
  int i;

  /* register baseline stats */
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions committed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores committed",
		   &sim_num_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_num_loads",
		   "total number of loads committed",
		   &sim_num_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_num_stores",
		   "total number of stores committed",
		   "sim_num_refs - sim_num_loads", NULL);
  stat_reg_counter(sdb, "sim_num_branches",
		   "total number of branches committed",
		   &sim_num_branches, /* initial value */0, /* format */NULL);
  /*stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);*/
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  stat_reg_counter(sdb, "sim_total_insn",
		   "total number of instructions executed",
		   &sim_total_insn, 0, NULL);
  stat_reg_counter(sdb, "sim_total_refs",
		   "total number of loads and stores executed",
		   &sim_total_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_total_loads",
		   "total number of loads executed",
		   &sim_total_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_total_stores",
		   "total number of stores executed",
		   "sim_total_refs - sim_total_loads", NULL);
  stat_reg_counter(sdb, "sim_total_branches",
		   "total number of branches executed",
		   &sim_total_branches, /* initial value */0, /* format */NULL);

  /* register performance stats */
#if 0
  stat_reg_counter(sdb, "sim_cycle",
		   "total simulation time in cycles",
		   &sim_cycle, /* initial value */0, /* format */NULL);
#endif
  stat_reg_formula(sdb, "sim_IPC",
		   "instructions per cycle",
		   "sim_num_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "sim_CPI",
		   "cycles per instruction",
		   "sim_cycle / sim_num_insn", /* format */NULL);
  stat_reg_formula(sdb, "sim_exec_BW",
		   "total instructions (mis-spec + committed) per cycle",
		   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "sim_IPB",
		   "instruction per branch",
		   "sim_num_insn / sim_num_branches", /* format */NULL);

  /* occupancy stats */
  stat_reg_counter(sdb, "IFQ_count", "cumulative IFQ occupancy",
                   &IFQ_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "IFQ_fcount", "cumulative IFQ full count",
                   &IFQ_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "ifq_occupancy", "avg IFQ occupancy (insn's)",
                   "IFQ_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "ifq_rate", "avg IFQ dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "ifq_latency", "avg IFQ occupant latency (cycle's)",
                   "ifq_occupancy / ifq_rate", /* format */NULL);
  stat_reg_formula(sdb, "ifq_full", "fraction of time (cycle's) IFQ was full",
                   "IFQ_fcount / sim_cycle", /* format */NULL);

  stat_reg_counter(sdb, "RUU_count", "cumulative RUU occupancy",
                   &RUU_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "RUU_fcount", "cumulative RUU full count",
                   &RUU_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "ruu_occupancy", "avg RUU occupancy (insn's)",
                   "RUU_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "ruu_rate", "avg RUU dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "ruu_latency", "avg RUU occupant latency (cycle's)",
                   "ruu_occupancy / ruu_rate", /* format */NULL);
  stat_reg_formula(sdb, "ruu_full", "fraction of time (cycle's) RUU was full",
                   "RUU_fcount / sim_cycle", /* format */NULL);

  stat_reg_counter(sdb, "LSQ_count", "cumulative LSQ occupancy",
                   &LSQ_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "LSQ_fcount", "cumulative LSQ full count",
                   &LSQ_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "lsq_occupancy", "avg LSQ occupancy (insn's)",
                   "LSQ_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "lsq_rate", "avg LSQ dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "lsq_latency", "avg LSQ occupant latency (cycle's)",
                   "lsq_occupancy / lsq_rate", /* format */NULL);
  stat_reg_formula(sdb, "lsq_full", "fraction of time (cycle's) LSQ was full",
                   "LSQ_fcount / sim_cycle", /* format */NULL);

  /* register predictor stats */
  if (pred)
    bpred_reg_stats(pred, sdb);

  /* register cache stats */
  if (cache_il1
      && (cache_il1 != cache_dl1 && cache_il1 != cache_dl2))
    cache_reg_stats(cache_il1, sdb);
  if (cache_il2
      && (cache_il2 != cache_dl1 && cache_il2 != cache_dl2))
    cache_reg_stats(cache_il2, sdb);
  if (cache_dl1)
    cache_reg_stats(cache_dl1, sdb);
  if (cache_dl2)
    cache_reg_stats(cache_dl2, sdb);
  if (itlb)
    cache_reg_stats(itlb, sdb);
  if (dtlb)
    cache_reg_stats(dtlb, sdb);

  /* debug variable(s) */
  stat_reg_counter(sdb, "sim_invalid_addrs",
		   "total non-speculative bogus addresses seen (debug var)",
                   &sim_invalid_addrs, /* initial value */0, /* format */NULL);

  for (i=0; i<pcstat_nelt; i++)
    {
      char buf[512], buf1[512];
      stat_stat_t *stat;

      /* track the named statistical variable by text address */

      /* find it... */
      stat = stat_find_stat(sdb, pcstat_vars[i]);
      if (!stat)
	fatal("cannot locate any statistic named `%s'", pcstat_vars[i]);

      /* stat must be an integral type */
      if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
	fatal("`-pcstat' statistical variable `%s' is not an integral type",
	      stat->name);

      /* register this stat */
      pcstat_stats[i] = stat;
      pcstat_lastvals[i] = STATVAL(stat);

      /* declare the sparce text distribution */
      sprintf(buf, "%s_by_pc", stat->name);
      sprintf(buf1, "%s (by text address)", stat->desc);
      pcstat_sdists[i] = stat_reg_sdist(sdb, buf, buf1,
					/* initial value */0,
					/* print format */(PF_COUNT|PF_PDF),
					/* format */"0x%lx %lu %.2f",
					/* print fn */NULL);
    }
  //ld_reg_stats(sdb);
  //  mem_reg_stats(mem, sdb);
}
