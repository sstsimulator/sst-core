//
// Definitions for Niagara MC simulator
//
/// @mainpage
/// McNiagara is a Monte Carlo model and simulator of the 
/// Niagara processor.
///
/// @section intro Introduction
/// This is an example of a main page section.
///

#ifndef MCNIAGARA_H
#define MCNIAGARA_H

#include <stdio.h>
#include <McSimDefs.h>
#include <OffCpuIF.h>
#include <MemoryModel.h>
#include <Dependency.h>
#include <CycleTracker.h>

// defining histogram lengths
#define  LD_LD_HIST_LENGTH     513
#define  ST_ST_HIST_LENGTH     513
#define  FP_FP_HIST_LENGTH     513
#define  INT_INT_HIST_LENGTH   513
#define  BR_BR_HIST_LENGTH     513
#define  ST_LD_HIST_LENGTH     513
#define  LD_USE_HIST_LENGTH    513      // load-to-use histogram
#define  INT_USE_HIST_LENGTH   513      // int_prodcucer-to-use histogram
#define  FP_USE_HIST_LENGTH    513      // fp-to-use histogram

#define  MAX_STB_ENTRIES   8

//--------- Simulation TERMINATION parameters
#define MAX_INST      (2*TOTAL_INSTS)   /*  Twice the number of instructions.
                                           Of course, CPI will converge 
                                           before getting there
                                           But this is jsut in case!  */
#define THRESHOLD      1.0E-3   /* Threshold on CPI variation tolerated */


/************************
 Info on the T2 Processor
 each core has a 16KB 8-way icache 32 byte lines
 8KB 4-way 16byte line L1 cache
 64 entry iTLB & 128entry DTLB
 All 8 cores share a unified 4MB 16-way L2 cache with 64 byte lines
 ************************/

//--------- Define latencies in cycles

/// Floating point unit latency?
#define FGU_LATENCY   6
/// Branch miss penalty
// Documentation states 6-cycle, but micro-benchmarks
// tell us that it's 7 cycles
#define BRANCH_MISS_PENALTY  7

/// L1 access latency is 3
// This is the basic latency, but it could vary depending
// on the distance to the next dep instruction/load
#define L1_LATENCY  3

/// L2 access latency.
// This is the basic latency, but it could vary depending
// on the distance to the next dep instruction/load 
//#define L2_LATENCY  23
#define L2_LATENCY  20

/// Main memory Latency
// (obtained from microbenchmark)
#define MEM_LATENCY  176

/// ITLB/DTLB miss latency
// This has not been affirmed yet, but is a close estimate
#define TLB_LATENCY  190

///
/// @brief Main simulator class
///
/// One object of this class is instantiated for each CPU
/// that should be simulated (SST needs to be able to simulate
/// multiple separate CPU's). See the main() function in 
/// McNiagara.cc for stand-alone execution.
///
class McNiagara {

   /// Structure to hold read-from-file model parameters
   struct ModelParam {
      char name[32];  ///< parameter string name
      union {
         unsigned long long lval;  ///< integer parameter ULL value
         double dval;              ///< real parameter double value
      } v;
   };
   
   ModelParam performanceCtr[20];  ///< parameters from perf_cnt.h (really 19)
   ModelParam instructionProb[30]; ///< parameters from inst_prob.h (really 28)
   
   enum InstructionProbIDs {
      PB_6_CTI_N, PB_6_INT_N, PB_25_INT_N, PB_6_FGU_N, PB_30_FGU_N,
      PB_6_MEM_N, PB_25_MEM_N, PB_3_LD_N, P_FDIV_FSQRT_S_N,
      P_FDIV_FSQRT_D_N, PB_6_INT_D_N, PB_25_INT_D_N, PB_6_FGU_D_N,
      PB_30_FGU_D_N, PB_6_MEM_D_N, PB_25_MEM_D_N, PB_3_LD_D_N,
      P_FDIV_FSQRT_S_D_N, P_FDIV_FSQRT_D_D_N, P_FDIV_FSQRT_S,
      P_FDIV_FSQRT_D, P_DS, DELAY_SLOT_N, ANNULLED_N, D_LOADS,
      D_STORES, D_FLOATS, D_INTS, NUM_INSTPROBS
   };
   // unfortunately must initialize strings in .cc file
   // they MUST correspond to the list above
   static const char* instructionProbNames[NUM_INSTPROBS+1];
   
   enum PerformanceCtrIDs {
      TOTAL_CYCLES, L2_MISSES, L2_I_MISSES, L1_MISSES, IC_MISSES,
      TLB_MISSES, ITLB_MISSES, TAKEN_BRS, TOTAL_INSTS, MEASURED_CPI,
      TOTAL_LDS, TOTAL_STS, TOTAL_FPS, TOTAL_BRS, LD_PERC, ST_PERC,
      BR_PERC, FP_PERC, GR_PERC, NUM_PERFCTRS
   };
   // unfortunately must initialize strings in .cc file
   // they MUST correspond to the list above
   /// Performance counter name strings
   static const char* performanceCtrNames[NUM_PERFCTRS+1];

   // JEC: Simpler instruction categorization: create an array
   // of instruction category probabilities in a CDF form, then
   // just gen a random (0,1) number and find where it lands
   /// General instruction types
   enum InstructionType {
      I_LOAD, I_STORE, I_BRANCH, I_GRPROD, I_FLOAT, I_NOP, I_NUMTYPES
   };
   /// Instruction type CDF
   double iTypeProbCDF[I_NUMTYPES];
   /// Delay slot instruction type CDF
   double iTypeDelaySlotProbCDF[I_NUMTYPES];

   /// Load instruction categories
   enum LoadCategory {
      PB_6_MEM, PB_25_MEM, PB_3_LD, OTHER_LD, NUMLOADCATS
   };
   /// Load category CDF
   double loadCatProbCDF[NUMLOADCATS];
   /// Delay slot load category CDF
   double loadCatDelaySlotProbCDF[NUMLOADCATS];

   /// Integer (GR) instruction categories
   enum IntCategory {
      PB_6_FGU, PB_30_FGU, PB_6_INT, PB_25_INT, OTHER_INT, NUMINTCATS
   };
   /// Int category CDF
   double intCatProbCDF[NUMINTCATS];
   /// Delay slot int category CDF
   double intCatDelaySlotProbCDF[NUMINTCATS];

   /// Float instruction categories
   enum FloatCategory {
      FDIV_FSQRT_S, FDIV_FSQRT_D, OTHER_FLOAT, NUMFLOATCATS
   };
   /// Float category CDF
   double floatCatProbCDF[NUMFLOATCATS];
   /// Delay slot float category CDF
   double floatCatDelaySlotProbCDF[NUMFLOATCATS];

   /// Token type structure for MC or trace tokens
   struct Token {
      InstructionType type;  ///< general type of instruction
      union {
         LoadCategory l;    ///< load category if load type
         IntCategory i;     ///< int/gr category if int type
         FloatCategory f;   ///< float category if float type
         unsigned int v;    ///< generic category value accessor
      } category;
      double optProb;    ///< optional probability for insn if needed
      bool inDelaySlot;  ///< true if instruction is in a delay slot
   };

   // --------- GLOBAL valriables

   //static char* stallReasonNames[NUMSTALLREASONS+1];
   // Effective stall time caused by each StallReason
   //double effective_t[NUMSTALLREASONS];
   CycleTracker cycleTracker;

   //enum token_type { IS_LOAD, IS_STORE };

   /// Data dependency tracking struct
   /*** REPLACED BY DependencyTracker CLASS
   struct dependency {
      unsigned long long which_inst; ///< Insn count of the DEPENDENT instruction
      double when_satisfied;         ///< Cycle number when the value is available
      enum StallReason reason;  ///< Reason for dependency
      struct dependency *next;  // Pointer to the next node
   };
   /// Linked list of data dependencies
   struct dependency *dc_head;
   ***/
   DependencyTracker depTracker;

   /// Memory operation queue (?)
   /*** REPLACED BY MemoryModel CLASS
   struct memory_queue {
      double when_satisfied;    // the cycle at which the mem op will be satisfied
      enum token_type whoami;   // Load or STORE
      struct memory_queue *next;
   };
   /// Linked list of memory operations
   struct memory_queue *mq_head;
   struct memory_queue *last_store;
   // How many loads/stores are currently in memory queue
   int ld_in_q, st_in_q;
   ***/
   MemoryModel memModel;

   // distance histograms for distances btween loads, stores, fp, int 
   // and branch instructions
   double ld_ld_hist[LD_LD_HIST_LENGTH],
          st_st_hist[ST_ST_HIST_LENGTH],
          fp_fp_hist[FP_FP_HIST_LENGTH],
          int_int_hist[INT_INT_HIST_LENGTH];
   double br_br_hist[BR_BR_HIST_LENGTH],
          st_ld_hist[ST_LD_HIST_LENGTH];

   // distance to use histograms ==> for dependencey checking
   double ld_use_hist[LD_USE_HIST_LENGTH],
          int_use_hist[INT_USE_HIST_LENGTH],
          fp_use_hist[FP_USE_HIST_LENGTH];

   double P1,
          P2,
          PM,
          PT,
          PG,
          PF,
          PBM,
          P_SP,
          PBR,
          PLD,
          PST;

   // Probabibilities in the Model Diagaram 
   double P_1,
          P_2,
          P_3,
          P_4,
          P_5,
          P_6,
          P_7,
          P_8,
          P_9,
          P_10,
          P_11,
          P_12,
          P_13;

   // Delay Slot Probabilities
   // - probabilities of what type of instruction in delay slot
   double PLD_D,
          PST_D,
          PF_D,
          PG_D;

   // Global variables
   FILE *outf;               ///< Output file handle
   double cycles;            ///< Global number of current cycle
   
   double probLoadFromSTB;

   //double p_FDIV_FSQRT_S,
   //       p_FDIV_FSQRT_D;

   // More variables: number of events, loads, stores, branches, ....etc
   unsigned long long n_loads,
        n_stores,
        n_memops,
        n_branches,
        n_miss_branches;
   unsigned long long n_l1,
        n_l2,
        n_mem,
        n_tlb,
        n_gr_produced,
        n_fr_produced;
   unsigned long long n_pipe_flushes,
        n_icache_misses,
        n_stb_full,
        n_stb_reads,
        //next_ld,
        last_fdiv;

   double total_stores,
          total_loads,
          total_gr_producers,
          total_fr_producers,
          total_int,
          total_fp,
          total_br,
          total_instructions;

   // Instruction cache miss probabilities
   double ic_p2,
          ic_pm,
          i_miss,
          ic_p1,
          ITLB_P,
          istall;

   /// Object pointer to external interface module 
   OffCpuIF *external_if;
   /// Input instruction trace, if in trace-driven mode
   FILE *tracef;

   bool Debug;

   unsigned long long tot_insns,
        tot_delayslot_insns;

   // one instruction per cycle is the max issue frequency
   // for a single-thread single-core model
   double CPIi;
   //       p_cpi;

 public:
   bool convergence;    ///< flag for model CPI convergence
   bool traceEnded;     ///< flag for trace file end
   McNiagara(void);      // constructor
   void init(const char *in_file, OffCpuIF * extif, const char *instProbFile, 
             const char *perfCountFile, const char *tracefile = 0,
             unsigned long seed=0);
   void un_init(void);
   int sim_cycle(unsigned long current_cycle);
   int generate_instruction(Token * token);
   int sim_instruction(Token * token);
   int fini(const char *outfile);
   //double my_rand(void);

 private:
   double make_cdf(double *buf, int length, int ignore_last_n, FILE * inf);
   int sample_hist(double *hist, int hist_length);
   int diff(double pred, unsigned long long real, int flag);
   void sanity_check(void);
   //double scan_memq(void);
   //void add_memq(double when_satisfied, enum token_type whoami);
   //double cycles_to_serve_load(enum StallReason *where, int flag, int d_dist,
   //      int l_dist);
   //void add_dependency(unsigned long long which_inst, double when_satisfied,
   //      enum StallReason reason);
   //void adjust_dependence_chain(double c);
   //double is_dependent(unsigned long long which_inst,
   //      enum StallReason *where);
   //void serve_store(void);
   //void handle_delay_slot(unsigned long long i,
   //      enum StallReason *last_ld_reason, double *last_ld_satisfied,
   //      double *fdiv_allowed);
   int read_paramfile(const char *filename, ModelParam params[],
                      const char *paramnames[]);

};

#endif
