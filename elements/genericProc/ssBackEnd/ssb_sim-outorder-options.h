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

#ifndef SSB_SIM_OUTORDER_OPTIONS_H
#define SSB_SIM_OUTORDER_OPTIONS_H

  /*
   * simulator options
   */
  //: maximum number of inst's to execute 
  unsigned int max_insts;
  //: number of insts skipped before timing starts 
  int fastfwd_count;
  //: Stop fast fwding instructions until this PC is encountered 
  word_t stop_PC;
  //: pipeline trace range and output filename 
  int ptrace_nelt;
  char *ptrace_opts[2];
  //: instruction fetch queue size (in insts) 
  int ruu_ifq_size;  
  //: extra branch mis-prediction latency 
  int ruu_branch_penalty;
  //: speed of front-end of machine relative to execution core 
  int fetch_speed;
  //: branch predictor type
  // nottaken,taken, perfect, bimod, or 2lev
  char *pred_type;
  //: bimodal predictor config (<table_size>) 
  int bimod_nelt;
  int bimod_config[1];
  //: 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) 
  int twolev_nelt;
  int twolev_config[4];
  //: combining predictor config (<meta_table_size> 
  int comb_nelt;
int comb_config[1];

  //: return address stack (RAS) size 
  int ras_size;

  //: BTB predictor config (<num_sets> <associativity>) 
  int btb_nelt;
int btb_config[2];

  //: instruction decode B/W (insts/cycle) 
  int ruu_decode_width;

  //: instruction issue B/W (insts/cycle) 
  int ruu_issue_width;

  //: run pipeline with in-order issue 
  int ruu_inorder_issue;

  //: issue instructions down wrong execution paths 
  int ruu_include_spec;

  //: instruction commit B/W (insts/cycle) 
  int ruu_commit_width;

  //: register update unit (RUU) size 
  int RUU_size;

  //: load/store queue (LSQ) size 
  int LSQ_size;

  //: l1 data cache config
  // i.e., <config>|none} 
  char *cache_dl1_opt;

  //: l1 data cache hit latency (in cycles) 
  int cache_dl1_lat;

  //: l2 data cache config
  //i.e., <config>|none
  char *cache_dl2_opt;

  //: l2 data cache hit latency (in cycles) 
  int cache_dl2_lat;

  //: l1 instruction cache config
  // i.e., <config>|dl1|dl2|none
  char *cache_il1_opt;

  //: l1 instruction cache hit latency (in cycles) 
  int cache_il1_lat;

  //: l2 instruction cache config,
  // i.e., <config>|dl1|dl2|none
  char *cache_il2_opt;

  //: l2 instruction cache hit latency (in cycles) 
  int cache_il2_lat;

  //: flush caches on system calls 
  int flush_on_syscalls;

  //: convert 64-bit inst addresses to 32-bit inst equivalents 
  int compress_icache_addrs;

  //: memory access latency (<first_chunk> <inter_chunk>) 
  int mem_nelt;
int mem_lat[2];

  //: memory access bus width (in bytes) 
  int mem_bus_width;

  //: instruction TLB config
  //  i.e., <config>|none
  char *itlb_opt;

  //: data TLB config,
  // i.e., <config>|none
  char *dtlb_opt;

  //: inst/data TLB miss latency (in cycles) 
  int tlb_miss_lat;

  //: total number of integer ALU's available 
  int res_ialu;

  //: total number of integer multiplier/dividers available 
  int res_imult;

  //: total number of memory system ports available (to CPU) 
  int res_memport;

  //: total number of floating point ALU's available 
  int res_fpalu;

  //: total number of floating point multiplier/dividers available 
  int res_fpmult;

//: text-based stat profiles 
#define MAX_PCSTAT_VARS 8
int pcstat_nelt;
char *pcstat_vars[MAX_PCSTAT_VARS];

#endif
