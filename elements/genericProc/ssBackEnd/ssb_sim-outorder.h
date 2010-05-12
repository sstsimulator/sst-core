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

#ifndef SSB_SIM_OUTORDER_H
#define SSB_SIM_OUTORDER_H

#include <set>
#include "ssb_memory.h"
#include "ssb_ruu.h"
#include "ssb_cv_link.h"
#include "ssb_rs_link.h"
#include "ssb_bitmap.h"
#include "ssb_cache.h"
#include "prefetch.h"
#include "sst/event.h"

#define GET_IMIX 1

class thread;

//: Conventional processor
//
// Base class for conventional processors, using simple scalar as the
// model.
//
//!SEC:ssBack
class convProc : public prefetchProc
{
  friend struct CV_link;

  /* 
   *  Typedefs
   */

  /* 
   * data members
   */ 

  long tickCount;
  processor *myProc;
  int myCoreID;
public:
  long long TimeStamp() const {
    return tickCount;
  }

  //: Check L2 Cache
  // 
  // Support for prefetcher
  virtual bool checkCache(const simAddress addr) {
    return cache_probe(cache_dl2, addr);
  };
  //: Insert to L2
  // 
  // Support for prefetcher
  virtual void insertCache(const simAddress mem) {
    bool dc; md_addr_t bumped = 0;
    cache_access(cache_dl2, Inject, (mem&~3), NULL, 0, TimeStamp(), 
		 NULL, NULL, dc, &bumped);
    if (dc == 1) {
      printf("need to handle main mem access in %s\n", __FILE__);
    }
    if (bumped != 0) {
      if (pref) pref->reportCacheEject(bumped);
    }
  }
  void wakeUpMM(map<instruction*, RUU_station*>::iterator &mi);
  void wakeUpPrefetched(instruction *inst);
  //: Send instruciton to Memory
  // 
  // Support for prefetcher
  virtual void sendToMem(instruction *i) {
    mmSendParcel(i);
  }
protected:
  //: simple fetching
  //
  // uses getNextInstruction() not getNextInstruction(PC). Useful for traces
  bool simpleFetch;

  //: max outstanding stores
  //
  // Maximum outstanding stores to main memory
  int maxMMStores;

  //: port limited commit
  //
  // 0 or -1 = do not use. Otherwise, this limits the number of
  // registers which can be written back in the commit stage. This is
  // in addition to the normal instruction limit (-commit:width).
  int portLimitedCommit;
  int regPortAvail;

  //: Extra cycles for WACI
  //
  // Allows the RdPort to be reserved for extra cycles by WACI Loads.
  int waciLoadExtra;
  
  unsigned long long lsqCompares;

  //: pipeline clear flag 
  //
  // are we trying to drain the pipeline? This flag is set to clear
  // the pipe, when switching threads or to take an 'interrupt'.
  bool clearPipe;

  //: Processor is serializing
  //
  // This flag indicates that the processor is serializing, most
  // likely due to a "sync" instruction. When this flag is set, the
  // fetch stage waits until the pipeline is clear before allowing any
  // new instructions to fecth.
  bool isSyncing;

  //: clock Ratio
  int clockRatio;

  //: Collection of active frames
  // space for allocated Frames for threads to use
  map<frameID, simRegister*> allocatedFrames;

  //: Set of Store instructions accessing main memory
  //
  // Instructions are recorded here before they are sent off to access
  // main memory. We keep them around here so that we can retire them
  // later.
  set<instruction*> mainMemStores;
  //: List of instructions waiting on stores to retire
  //
  // Because we need to keep remote stores around, we can't retire
  // them where we normally would. Plus, we need to retire everything
  // in-order. Thus, whenever we have remote stores we queue up
  // everyone who can retire as soon as that store gets back.
  deque<instruction*> retireList;
  //: Set of stores which have arrived out of order
  //
  // If remote stores arrive out of order, we need to keep track of
  // who we can and can't retire.
  set<instruction*> OOOStores;

  set<instruction*> condemnedRemotes;

  //: Map of Load instructions accessing main memory
  //
  // Loads Instructions are recorded here before they are sent off to
  // access main memory. When the return, we take them from here and
  // put them in the event queue for writeback.
  map<instruction*, RUU_station*> mainMemLoads;

  //: Instruction blocking ifetch
  //
  // An instruction being fetched which requires a remote
  // access. Until this instruction returns, ifetching is
  // blocked. When it is returned, we squash it and restart it.
  instruction *iFetchBlocker;

  //: Memory Controller
  // 
  // Where we send memory requests when they miss cache
  //component *memCtrl;

  prefetcher *pref;

  //: Thread of execution
  thread *thr;
  
  //: size of an instruction 
  //
  // in bytes
  int instructionSize;

  //: Simple memory model flag
  //
  // 'simpleMemory' uses just the simple scalar memory model (i.e., no
  // external memory accesses, main memory is a constant
  // latency). Otherwise, we model remote accesses with a memory
  // controller.
  int simpleMemory;

  RS_link_list rs_free_list;

  //: Last op attempted to dispatch
  //
  // the last operation that ruu_dispatch() attempted to dispatch, for
  // implementing in-order issue
  struct RS_link last_op;

  //: program counter 
  md_addr_t pred_PC;

  //: PC to recover to
  md_addr_t recover_PC;
  
  //: fetch unit next fetch address 
  md_addr_t fetch_regs_PC;

  //: predicted fetch PC
  md_addr_t fetch_pred_PC;

  //: IFETCH -> DISPATCH inst queue 
  struct fetch_rec *fetch_data;
  //: num entries in IF -> DIS queue 
  int fetch_num;			
  //: tail pointer of fetch queue 
  int fetch_tail;
  //: head pointer of fetch queue 
  int fetch_head;	
  
  //: Did the last instruction miss
  int last_inst_missed;
  int last_inst_tmissed;

  //: register update unit
  //
  // register update unit, combination of reservation stations and
  // reorder buffer device, organized as a circular queue 
  struct RUU_station *RUU;		
  //: RUU head pointer
  int RUU_head;
  //: RUU tail pointer
  int RUU_tail;	
  //: num entries in RUU
  int RUU_num;

  //: load/store queue (LSQ):
  //
  // load/store queue (LSQ): holds loads and stores in program order,
  // indicating status of load/store access:
  //
  // 
  // - issued: address computation complete, memory access in progress
  //
  // - completed: memory access has completed, stored value available
  //
  // - squashed: memory access was squashed, ignore this entry
  //
  // loads may execute when:
  //
  //   1) register operands are ready, and
  //
  //   2) memory operands are ready (no earlier unresolved store)
  //
  // loads are serviced by:
  //
  // 1) previous store at same address in LSQ (hit latency), or
  //
  // 2) data cache (hit latency + miss latency)
  //
  // stores may execute when:
  //
  // 1) register operands are ready
  //
  // stores are serviced by: 
  //
  // 1) depositing store value into the load/store queue
  //
  // 2) writing store value to the store buffer (plus tag check) at commit
  //
  // 3) writing store buffer entry to data cache when cache is free
  //
  // NOTE: the load/store queue can bypass a store value to a load in
  // the same cycle the store executes (using a bypass network), thus
  // stores complete in effective zero time after their effective
  // address is known
  struct RUU_station *LSQ;         /* load/store queue */
  //: LSQ head pointer
  int LSQ_head;
  //: LSQ tail pointer
  int  LSQ_tail;
  //: num entries currently in LSQ 
  int LSQ_num;        

  //: Pending event queue
  //
  // pending event queue, sorted from soonest to latest event (in
  // time), NOTE: RS_LINK nodes are used for the event queue list so
  // that it need not be updated during squash events 
  struct RS_link *event_queue;

  //the ready instruction queue
  struct RS_link *ready_queue;  

  unsigned int use_spec_cv[CV_BMAP_SZ];
  //: Create Vector
  //
  // the create vector maps a logical register to a creator in the RUU
  // (and specific output operand) or the architected register file
  // (if RS_link is NULL)
  //
  // Note: speculative copy on write storage provided for fast
  // recovery during wrong path execute (see tracer_recover() for
  // details on this process
  struct CV_link create_vector[MD_TOTAL_REGS+2];
  //: Speculative create vector
  //
  // indicates create in speculative state
  struct CV_link spec_create_vector[MD_TOTAL_REGS+2];
  
  //: Indicate when a register was created
  tick_t create_vector_rt[MD_TOTAL_REGS+2];
  //: Indicate when a speculative register was created
  tick_t spec_create_vector_rt[MD_TOTAL_REGS+2];  

  /*
   * simulator options
   */
  #include "ssb_sim-outorder-options.h"
  
  /* options database */
  //: Simulator options
  struct opt_odb_t *sim_odb;

  /*
   * simulator stats
   */
  //: stats database 
  struct stat_sdb_t *sim_sdb;

#if GET_IMIX == 1
  //: instruction Mix counters
  unsigned long long iMix[LASTINST];
#endif
  //: Number of instructions executed
  counter_t sim_num_insn;
  //: total number of instructions executed
  counter_t sim_total_insn;
  //: total number of memory references committed 
  counter_t sim_num_refs;
  //: total number of memory references executed 
  counter_t sim_total_refs;
  //: total number of loads committed 
  counter_t sim_num_loads;
  //: total number of loads executed 
  counter_t sim_total_loads;
  //: total number of branches committed 
  counter_t sim_num_branches;
  //: total number of branches executed 
  counter_t sim_total_branches;
  //: cumulative IFQ occupancy 
  counter_t IFQ_count;
  //: cumulative IFQ full count
  counter_t IFQ_fcount;
  //: cumulative RUU occupancy 
  counter_t RUU_count;
  //: cumulative RUU full count
  counter_t RUU_fcount;
  //: cumulative LSQ occupancy 
  counter_t LSQ_count;
#define WANT_LSQ_HIST 1
#if WANT_LSQ_HIST == 1
  map<int, counter_t> LSQ_hist;
#endif
  //: cumulative LSQ full count
  counter_t LSQ_fcount;
  // total non-speculative bogus addresses seen
  // (debug var) 
  counter_t sim_invalid_addrs;
  
  /*
   * simulator state variables
   */  
  //: execution start times 
  time_t sim_start_time;
  //: execution end times 
  time_t sim_end_time;
  //: elapsed sim time
  int sim_elapsed_time;

  //: instruction sequence counter
  //
  // instruction sequence counter, used to assign unique id's to insts
  unsigned int inst_seq;
  //: pipetrace instruction sequence counter 
  unsigned int ptrace_seq;
  //: Speculation mode
  //
  // speculation mode, non-zero when mis-speculating, i.e., executing
  // instructions down the wrong path, thus state recovery will
  // eventually have to occur that resets processor register and
  // memory state back to the last precise state
  bool spec_mode;
  //: encountered a lmw or stmw 
  int lsq_mult;
  //: cycles until fetch issue resumes 
  // for delays caused by l1 or tlb misses
  unsigned ruu_fetch_issue_delay;
  //: cycles till dispatch resumes
  // for delays caused by FEB misses.
  unsigned ruu_dispatch_delay;
  virtual unsigned getFEBDelay() {return 0;}
  //: perfect prediction enabled 
  int pred_perfect;
  //: speculative bpred-update enabled 
  char *bpred_spec_opt;
  //: Speculation enum
  enum { spec_ID, spec_WB, spec_CT } bpred_spec_update;
  //: iL1
  // level 1 instruction cache, entry level instruction cache 
  struct cache_t *cache_il1;
  //: iL2
  // level 2 instruction cache 
  struct cache_t *cache_il2;
  //: dL1
  // level 1 data cache, entry level data cache 
  struct cache_t *cache_dl1;
  //: dL2
  // level 2 data cache 
  struct cache_t *cache_dl2;
  //: instruction TLB 
  struct cache_t *itlb;
  //: data TLB 
  struct cache_t *dtlb;
  //: branch predictor 
  struct bpred_t *pred;
  //: functional unit resource pool 
  struct res_pool *fu_pool;
  //: text-based stat profiles 
  struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
  //: text-based stat profiles 
  counter_t pcstat_lastvals[MAX_PCSTAT_VARS];
  //: text-based stat profiles 
  struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];

  typedef map<instruction*, int> latencyMap;
  latencyMap extraInstLat;
  instruction *committingInst;

  /* 
   * ssb_sim-outorder-constructor.h
   */ 
public:
  convProc(string configFile, processor *p, int maxMMOut, int coreNum);
  //component *getMemCtrl() {return memCtrl;}
protected:

  /* 
   *  memory and TLB access functions. located in
   *  ssb_sim-outorder-memory.cc
   */
  void mmSendParcel(instruction *inst);
  void mainMemAccess(instruction*);
  unsigned int mem_access_latency(int blk_sz);
  virtual unsigned int cplx_mem_access_latency(const enum mem_cmd cmd,
					       const md_addr_t baddr,
					       const int bsize,
					       bool &);
  /*enum md_fault_type new_mem_access(struct mem_t *mem, enum mem_cmd cmd,
				    md_addr_t addr, void *vp,
				    int nbytes);*/
  virtual void noteWrite(const simAddress a) {;};
  uint dl1_access_fn(enum mem_cmd cmd,  md_addr_t baddr, int bsize,
		     struct cache_blk_t *blk,  tick_t now, bool&);
  uint dl2_access_fn(enum mem_cmd cmd,  md_addr_t baddr,  int bsize,		
		     cache_blk_t *blk, tick_t now, bool&);
  uint il1_access_fn(enum mem_cmd cmd,	md_addr_t baddr, int bsize,	
		     struct cache_blk_t *blk, tick_t now, bool&);
  uint il2_access_fn(enum mem_cmd cmd,	md_addr_t baddr,  int bsize,	
		     struct cache_blk_t *blk, tick_t now, bool&);
  uint itlb_access_fn(enum mem_cmd cmd,	md_addr_t baddr, int bsize,		
		      struct cache_blk_t *blk,	tick_t now, bool&);
  uint dtlb_access_fn(enum mem_cmd cmd,	md_addr_t baddr, int bsize,	
		      struct cache_blk_t *blk, tick_t now, bool&);

  /* 
   *  ssb_main.cc
   */
  void sim_print_stats(FILE *fd);
  int ss_main(const char*);

  /* 
   * options & stats located in ssb_sim-outorder-options.cc
   */
  void sim_reg_options(struct opt_odb_t *odb);
  void sim_check_options(struct opt_odb_t *odb);
  void sim_reg_stats(struct stat_sdb_t *sdb);

  /* 
   *  init stuff ssb_sim-outorder-init.cc
   */ 
  void sim_init(void);
  // note load_prog also does other init stuff
  void sim_load_prog(const string fuConfStr);
  void ruu_init(void);
  void lsq_init(void);
  void fetch_init(void);

  /* 
   *  dump stuff ssb_sim-outorder-dump.cc
   */ 
  static void ruu_dumpent(struct RUU_station *rs, int index, FILE *stream,
			  int header);
  void ruu_dump(FILE *stream);
  void lsq_dump(FILE *stream);
  void rspec_dump(FILE *stream);
  void mspec_dump(FILE *stream);
  void fetch_dump(FILE *stream);
  void eventq_dump(FILE *stream);
  void readyq_dump(FILE *stream);

  /* 
   * DLite stuff ssb_sim-outorder-dlite.cc
   */ 
  static char *simoo_reg_obj(struct regs_t *regs, int is_write,
			     enum md_reg_type rt, int reg,
			     struct eval_value_t *val);
  static char *simoo_mem_obj(struct mem_t *mem,	int is_write,
			     md_addr_t addr, char *p, int nbytes);		
  static char *simoo_mstate_obj(FILE *stream, char *cmd, struct regs_t *regs,	
				struct mem_t *mem);  

  /*
   * execution unit event queue - ssb_sim-outorder-eventq.cc
   */
  struct RUU_station *eventq_next_event(void);
  void eventq_queue_event(struct RUU_station *rs, tick_t when);
  void eventq_init(void);

  /*
   * The ready instruction queue - ssb_sim-outorder-readyq.cc
   */
  void readyq_init(void);
  void readyq_enqueue(struct RUU_station *rs);

  /* 
   * Tracer functions - ssb_sim-outorder-tracer.cc
   */
  void tracer_recover(void);
  void tracer_init(void);
  void ruu_recover(int branch_index);

  /* 
   * idep/odep handling - ssb_sim-outorder-dep.cc
   */
  void ruu_link_idep(struct RUU_station * const rs, const int idep_num, const int idep_name);
  void ruu_install_odep(struct RUU_station *rs,	int odep_num, int odep_name);

  /* 
   *  "main loop" simulation functions
   */
  void sim_loop(void);
  void ruu_release_fu(void);
  void ruu_commit(void);
  void ruu_writeback(void);
  void lsq_refresh(void);
  void ruu_issue(void);
  void ruu_dispatch(void);
  void ruu_fetch(void);
public:
  //: Check if the pipeline is clear
  //
  // NOTE: we could probabl speed this up by tagging entries in the
  // retireList with the thread to which they belong. That way, we
  // could just check the RUU and fetch, instead of checking the
  // retire list and having to wait for all stores to return.
  bool pipeClear() {return (RUU_num == 0 && fetch_num == 0 &&
			    retireList.empty());}
protected:
  //: Check if the pipeline past the fetch stage is clear
  //
  // this is the same as pipeClear(), except it ignores instructions
  // in the fetch->dispatch pipe. This is used for sync instructions.
  bool pipeDispatchClear() {return (RUU_num == 0 && retireList.empty());}

  /* 
   * Enkidu functions 
   */ 
  virtual void setup()=0;
  virtual void finish();
public:
  virtual void handleMemEvent(instruction* inst );
protected:
  virtual void preTic()=0;
  virtual void postTic()=0;
  void handleReturningStore(instruction *inst);

  /* 
   * Processor functions
   */
  frameID requestFrame(int size);
  simRegister* getFrame(frameID);
  void returnFrame(frameID);
  virtual bool insertThread(thread*);
  //bool isLocal(const simAddress, const simPID);
  void dataCacheInvalidate( simAddress addr );

};

/* read a create vector entry */
#define CREATE_VECTOR_P(P,N)  (BITMAP_SET_P(P->use_spec_cv, CV_BMAP_SZ, (N)) \
			       ? P->spec_create_vector[N]		\
			       : P->create_vector[N])
#define CREATE_VECTOR(N)        (BITMAP_SET_P(use_spec_cv, CV_BMAP_SZ, (N))\
				 ? spec_create_vector[N]                \
				 : create_vector[N])

/* read a create vector timestamp entry */
#define CREATE_VECTOR_RT(N)     (BITMAP_SET_P(use_spec_cv, CV_BMAP_SZ, (N))\
				 ? spec_create_vector_rt[N]             \
				 : create_vector_rt[N])

/* set a create vector entry */
#define SET_CREATE_VECTOR(N, L) (spec_mode                              \
				 ? (BITMAP_SET(use_spec_cv, CV_BMAP_SZ, (N)),\
				    spec_create_vector[N] = (L))        \
				 : (create_vector[N] = (L)))

/* specified instruction is a LMW or STMW or other variants */
#ifdef TARGET_PPC
//#define  IS_MULT_LSQ(op) ((op == STMW) || (op == LMW) || (op == LSWI) || (op == LSWX) || (op == STSWI) || (op == STSWX))
#define  IS_MULT_LSQ(op) 0
#else
#define  IS_MULT_LSQ(op) FALSE
#endif

#endif
