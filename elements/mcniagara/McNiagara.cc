// By:Waleed Al-Kohlani
//
// Major restructuring changes: Jonathan Cook
// - cleanup indentation (horrible!)
// - move instruction processing into separate routine
// - combine normal and delay slot instruction processing
// - have a true token generator
// - have an MC token generator and a trace-file token generator
// -- all above is done as of 3:00pm, 22 Sep 2009
// - now objectify it
// -- is compiling and running as class/object, but not very clean yet
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include "McNiagara.h"

#include "mersenne.h"           // The code for the MERSENNE PRNG
//#include "perf_cnt.h"           // Performance Counters data!
//#include "inst_prob.h"          // Instruction Special Probabilities

//
// Name strings for the parameters read in from files
//
const char* McNiagara::instructionProbNames[NUM_INSTPROBS+1] = {
   "PB_6_CTI_N", "PB_6_INT_N", "PB_25_INT_N", "PB_6_FGU_N",
   "PB_30_FGU_N", "PB_6_MEM_N", "PB_25_MEM_N", "PB_3_LD_N",
   "P_FDIV_FSQRT_S_N", "P_FDIV_FSQRT_D_N", "PB_6_INT_D_N",
   "PB_25_INT_D_N", "PB_6_FGU_D_N", "PB_30_FGU_D_N", "PB_6_MEM_D_N",
   "PB_25_MEM_D_N", "PB_3_LD_D_N", "P_FDIV_FSQRT_S_D_N",
   "P_FDIV_FSQRT_D_D_N", "P_FDIV_FSQRT_S", "P_FDIV_FSQRT_D",
   "P_DS", "DELAY_SLOT_N", "ANNULLED_N", "D_LOADS", "D_STORES",
   "D_FLOATS", "D_INTS", 0 };
const char* McNiagara::performanceCtrNames[NUM_PERFCTRS+1] = {
   "TOTAL_CYCLES", "L2_MISSES", "L2_I_MISSES", "L1_MISSES", "IC_MISSES",
   "TLB_MISSES", "ITLB_MISSES", "TAKEN_BRS", "TOTAL_INSTS",
   "MEASURED_CPI", "TOTAL_LDS", "TOTAL_STS", "TOTAL_FPS", "TOTAL_BRS",
   "LD_PERC", "ST_PERC", "BR_PERC", "FP_PERC", "GR_PERC", 0 };

///
/// @brief Constructor: initializes lots of stuff
///
/// Basically all of the old global var initializations
/// are here now.
///
McNiagara::McNiagara(void)
{
   Debug = false;
   cycles = 0;
   n_loads = 0;
   n_stores = 0;
   n_memops = 0;
   n_branches = 0;
   n_miss_branches = 0;
   n_l1 = 0;
   n_l2 = 0;
   n_mem = 0;
   n_tlb = 0;
   n_gr_produced = 0;
   n_fr_produced = 0;
   n_pipe_flushes = 0;
   n_icache_misses = 0;
   n_stb_full = 0;
   n_stb_reads = 0;
   last_fdiv = 0;
   ic_p2 = 0;
   ic_pm = 0;
   i_miss = 0;
   ic_p1 = 0;
   ITLB_P = 0;
   istall = 0;
   tot_insns = 0;
   tot_delayslot_insns = 0;
   CPIi = 1.0;
   tracef = 0;
   convergence = false;
   traceEnded = false;
}

///
/// @brief Wrapper for Mersenne PRNG
///
/// The Mersene Twister random number generator is
/// one of the best state-of-the-art PRNG
/// 
/// @return a uniform random number in [0 1]
///
double my_rand(void)
{
   double r = (double) genrand_real2(); // this function is defined in mersene.h
   return r;
}

///
/// @brief Read a CDF from a file
///
/// This reads part of a CDF input file into a CDF array;
/// This should really be fixed so that the file has array
/// delimiters rather than just a bunch of number lines.
/// @param buf is the array to fill with the CDF values
/// @param length is the number of values to read in 
/// @param ignore_last_n is the number of values at end
///        to NOT add in to the accumulated sum (returned)
/// @param inf is the FILE * to read from
/// @return the cumulative sum of the values (less ignored)
///
double McNiagara::make_cdf(double *buf, int length, int ignore_last_n, FILE * inf)
{
   unsigned long long val;
   double cumsum;
   int i,
       err_chk;

   for (cumsum = 0.0, i = 0; i < length; i++) {
      err_chk = fscanf(inf, "%llu", &val);
      assert(err_chk != 0 && err_chk != EOF);
      if (i < (length - ignore_last_n))
         cumsum += (double) val;
      buf[i] = cumsum;
   }

   // Make a CDF
   for (i = 0; i < length; i++) {
      buf[i] = buf[i] / cumsum;
   }

   // the total number of occurences of the instruction is returned! 
   return cumsum;
}

///
/// @brief Sample CDF histogram and return bin index
///
/// This will return a number(instruction) 
/// probablistically/randomlly which would tell us
/// when the next load,st,fp,int,br will occour; or when the
/// dependent instruction on the current one will be at. 
/// This function utilizes a CDF array made earlier
/// @param hist is a CDF histogram array
/// @param hist_length is the number of elements in the CDF
/// @return the bin index selected (0 to length-1)
///
int McNiagara::sample_hist(double *hist, int hist_length)
{
   register int i;
   double r;

   r = my_rand();

   for (i = 0; i < hist_length; i++) {
      if (hist[i] >= r)
         break;
   }

   return i;
}

///
/// @brief Check predicted versus real performance counts
///
/// This function returns 0 if the difference between
/// performance counts and collected Shade counts
/// is okay (<2% for each category of instructions & <5% for 
/// total_instruction count). It returns 1 otherwise. 
/// @param pred is the predicted (model calculated) value
/// @param real is the real measured value
/// @param flag is a flag (??)
/// @return 0 if difference is acceptable, 1 otherwise
///
int McNiagara::diff(double pred, unsigned long long real, int flag)
{
   // 2% difference is max allowed difference for each type of instructions
   // This is crucial so that we get probabilities as close as possible to
   // the real mix of instructions
   double b = 2.0;

   double diff = (double) 100 * (pred - real) / (real);

   if (diff < 0)
      diff = diff * -1;         // resetting the negative

   // This is the special case of FGU instructions in INT programs
   // Performance Counts in purely INT programs count few FGU instructions but
   // shade counts 0 FGU instructions
   // Therefore, the difference would be 100% but is in fact negligible so we
   // take care of that here
   if (diff == 100 && flag == 2)
      return 1;

   // If difference between total instructions is > 20% then there's
   // definately a big problem  
   if (flag == 1 && diff >= 20.0) {
      printf("Error: The difference %2.4f is too big.\n"
            "Please make sure the input data are reasonable\n.", diff);
      exit(1);
   }
   // If difference between total instructions is > 5% and < 20% ==> attemp to fix it
   if (flag == 1)
      b = 5.0;

   if (diff <= b)
      return 0;
   else
      return 1;

}

///
/// @brief Check performance counts (???)
///
/// This function will check Shade-collected counts against
/// performance-counters-collected counts
/// If the difference is small enough (see diff() above ), it will try
/// to fix the count
/// If the difference is too big, the program will exit with an error
///
void McNiagara::sanity_check()
{

   // Checking numbers
   printf("Difference between Instrumentation and counters ::\n");
   printf("If you see \"Fixing....\", then you might have a problem in the collected data\n");
   printf("An attempt will be made to fix this, but if the result error is too big, then\n");
   printf("You should revisit this and check why Shade-collected numbers "
         "differ from performance-counters numbers\n");

   printf("Sanity Checks ..... \n");

   if (diff(total_instructions, performanceCtr[TOTAL_INSTS].v.lval, 1) == 0)
      printf("Total Instructions ==> Okay\n");
   else {
      printf("Total Instructions ==> Fixing....\n");
      total_instructions = performanceCtr[TOTAL_INSTS].v.lval;
   }

   if (diff(total_loads, performanceCtr[TOTAL_LDS].v.lval, 0) == 0)
      printf("..");
   else {
      total_loads = performanceCtr[TOTAL_LDS].v.lval;
   }

   if (diff(total_stores, performanceCtr[TOTAL_STS].v.lval, 0) == 0)
      printf("..");

   else {
      total_stores = performanceCtr[TOTAL_STS].v.lval;
   }

   if (diff(total_fp, performanceCtr[TOTAL_FPS].v.lval, 2) == 0)
      printf("..");
   else {
      total_fp = performanceCtr[TOTAL_FPS].v.lval;
   }

   if (diff(total_br, performanceCtr[TOTAL_BRS].v.lval, 0) == 0)
      printf("..");
   else {
      total_br = performanceCtr[TOTAL_BRS].v.lval;
   }

   total_int =
         total_instructions - total_br - total_loads - total_stores -
         total_fp;
   total_instructions =
         total_loads + total_stores + total_fp + total_int + total_br;
   if (diff(total_instructions, performanceCtr[TOTAL_INSTS].v.lval, 1) != 0) {
      printf("Total Instructions ==> Fixing ...\n");
      sanity_check();           // keep doing this until the error is fixed or we abort. 
   } else {
      printf("\nTotal Instructions ==> All Okay\n");

   }

}

///
/// @brief Read a file of "\#define SYM val" lines into a parameter array
///
/// This reads a previously \#include'd parameter file into an array of
/// values. It expects \#define lines where each val is either an integer
/// ending in "ULL" or a real value with a decimal point in it. It searches
/// the string representation of the value for "ULL" or "." and based on 
/// the match converts it to an unsigned long long or a double. It then 
/// searches the paramnames argument for a matching parameter name, and if
/// found sets the corresponding index in the params argument.
/// @param filename is the file to open and read from
/// @param params is the parameter array (assumed to be same size as paramnames)
/// @param paramnames is an array of parameter names to read in
/// @return 0 upon success, nonzero on error
/// 
int McNiagara::read_paramfile(const char *filename,
                              McNiagara::ModelParam params[],
                              const char *paramnames[])
{
   int i;
   FILE *f;
   char line[256];
   unsigned long long lv;
   double dv;
   char name[40], value[40];
   enum { NONE, ULL, DBL } valtype;
   f = fopen(filename,"r");
   if (!f) 
      return 1;
   while (fgets(line,sizeof(line),f)) {
      valtype = NONE;
      lv = 0;
      dv = 0;
      name[39] = value[39] = '\0';
      if (sscanf(line,"#define %39s %39s", (char*) &name, (char*) &value) == 2) {
         //printf("matched (%s) (%s)\n",name,value);
         if (strstr(value,"L")) {
            sscanf(value, "%llu", &lv);
            //printf("ull val = %llu\n",lv);
            valtype = ULL;
         } else if (strstr(value,".")) {
            sscanf(value, "%lg", &dv);
            //printf("dbl val = %lg\n",dv);
            valtype = DBL;
         }
         for (i=0; paramnames[i]; i++)
            if (!strcmp(paramnames[i], name))
               break;
         if (paramnames[i] && valtype != NONE) {
            strcpy(params[i].name, paramnames[i]);
            if (valtype == ULL)
               params[i].v.lval = lv;
            else if (valtype == DBL)
               params[i].v.dval = dv;
            //printf("init %s %llu %g\n", params[i].name, lv, dv);
         }
      }
   }
   fclose(f);
   return 0;
}

///
/// @brief Initialize model 
///
/// This function reads in parameter and CDF files and initializes the
/// variables (probabilities)
/// @param in_file is the input CDF file
/// @param extif is an off-cpu interface object (for SST reporting of
///        mem/nic events, but might be useful elsewhere too)
/// @param tracefile is an instruction trace file if a trace-driven
///        simulation is wanted (otherwise, pure Monte Carlo)
///
void McNiagara::init(const char *in_file, OffCpuIF * extif, 
                     const char *instProbFile, const char *perfCountFile,
                     const char *tracefile, unsigned long seed)
{
   double p1,
          p2,
          pm;
   double totalLoadsFromSTB;
   FILE *inf;

   external_if = extif;
   
   // Read in instruction count and performance counter parameters
   // from files (rather than #include'ing them)
   read_paramfile(instProbFile, instructionProb, instructionProbNames);
   read_paramfile(perfCountFile, performanceCtr, performanceCtrNames);

   // Reading Histogram Files Below and Making CDF arrays
   inf = fopen(in_file, "r");
   if (inf == NULL) {
      fprintf(stderr, "McNiagara: Error opening input file %s\n", in_file);
      exit(0);
   }
   // Read load-to-use histogram
   make_cdf(ld_use_hist, LD_USE_HIST_LENGTH, 0, inf);

   // Read int-to-use histogram
   total_gr_producers = make_cdf(int_use_hist, INT_USE_HIST_LENGTH, 0, inf);

   // Read fp-to-use histogram
   total_fr_producers = make_cdf(fp_use_hist, FP_USE_HIST_LENGTH, 0, inf);

//-----------------------------------------------------------------//   
   // read ld-ld histogram
   total_loads = make_cdf(ld_ld_hist, LD_LD_HIST_LENGTH, 0, inf);

   // read st-st histogram
   total_stores = make_cdf(st_st_hist, ST_ST_HIST_LENGTH, 0, inf);

   // read fp-fp histogram
   total_fp = make_cdf(fp_fp_hist, FP_FP_HIST_LENGTH, 0, inf);

   // read int-int histogram
   total_int = make_cdf(int_int_hist, INT_INT_HIST_LENGTH, 0, inf);

   // read br-br histogram
   total_br = make_cdf(br_br_hist, BR_BR_HIST_LENGTH, 0, inf);

   // Read store-to-load histogram
   totalLoadsFromSTB = make_cdf(st_ld_hist, ST_LD_HIST_LENGTH, 0, inf);
   //printf("P(load from STB): %g\n", probLoadFromSTB);
   
   fclose(inf);

   // Now, we add CTI (Control Transfer Instructions ) to regular branch instructions
   // This is crucial so as to correctly consider branch instructions.
   // CTI instructions are counted as branch instructions and they act as branch
   // instructions too
   // JEC: This is because the collection tool miscategorizes? Why not fix the tool?
   total_br += instructionProb[PB_6_CTI_N].v.lval;
   total_int = total_int - instructionProb[PB_6_CTI_N].v.lval;  // This is done for the same reason
   
   total_instructions =
         total_loads + total_stores + total_fp + total_int + total_br;

   sanity_check();              // check if our numbers are correct 
   total_instructions =
         total_loads + total_stores + total_fp + total_int + total_br;

/************
      Model Chart Probabilities
    
P_1 == Probability of an instruction that does NOT retire immediately
P_2 == Probability of an instruction that retires immediately
P_3 == Probability of an instruction being  a branch instruction
P_4 == Probability of an instruction being  a floating point instruction
P_5 == Probability of an instruction being  a fp fdiv/fsqrt instruction
P_6 == Probability of an instruction being  a fp instruction NOT fdiv/fsqrt
P_4 == P_5 + P_6

P_7 == Probability of an instruction being an INT instruction that has a 
       latency of more than 1 cycle
P_8 == Probability of an instruction being a load instruction
P_9  == Probability of an instruction being a load that does not miss the DTLB
P_10 ==   Probability of an instruction being a load that misses the DTLB
P_8 = P_9 + P_10
  
P_11 == Probability of an instruction being a load that hits in the L1 cache
P_12 == Probability of an instruction being a load that hits in the L2 cache
P_13 == Probability of an instruction being a load that hist in main memory
   
P_1 = P_3 + P_4 + P_7 + P_8 ; 
P_2 = 1 - P_1 ; 
**************/

   // ----- Memory Probabilities
   // Probability of load being satisfied in L2
     // need # here and possibly correct it
   p2 = (performanceCtr[L1_MISSES].v.lval - performanceCtr[L2_MISSES].v.lval)
        / total_loads;

   // Probability of load being satisfied in memory
   pm = (performanceCtr[L2_MISSES].v.lval) / total_loads; // or just L2_MISSES/total_loads; 

   // Probability of load being satisfied in L1
   p1 = 1.0 - (p2 + pm);
   // Probability of load being satisfied in L1
   P1 = p1;

   // Given L1 miss, Probability of load being satisfied in L2
   P2 = p2 / (1 - p1);

   // Given L1, L2  miss, Probability of load being satisfied in Memory (must = 1)
   PM = pm / (1 - p1 - p2);
   // Probability of TLB miss for 
   PT = ((double) 1.0 * performanceCtr[TLB_MISSES].v.lval / (performanceCtr[TOTAL_LDS].v.lval));

   P_9 = (double) 1.0 * (total_loads - performanceCtr[TLB_MISSES].v.lval) / total_instructions;
   P_10 = (double) 1.0 * performanceCtr[TLB_MISSES].v.lval / total_instructions;
   P_11 = (double) 1.0 * (total_loads - performanceCtr[L1_MISSES].v.lval) / total_instructions;
   P_12 = (double) 1.0 * (performanceCtr[L1_MISSES].v.lval - performanceCtr[L2_MISSES].v.lval) /
                         total_instructions;
   P_13 = (double) 1.0 * (performanceCtr[L2_MISSES].v.lval) / total_instructions;

   probLoadFromSTB = totalLoadsFromSTB / total_loads;

   // calculating IC miss rates
   i_miss = (double) 1.0 * (performanceCtr[IC_MISSES].v.lval) / total_instructions;
   ic_p2 = (double) 1.0 * (performanceCtr[IC_MISSES].v.lval - 
                           performanceCtr[L2_I_MISSES].v.lval) / 
                          performanceCtr[IC_MISSES].v.lval;
   ic_pm = (double) 1.0 * (performanceCtr[L2_I_MISSES].v.lval) / 
                          performanceCtr[IC_MISSES].v.lval;

   ic_p1 = 1.0 - (ic_p2 + ic_pm);

   //ITLB_P = ((double) 1.0 * performanceCtr[ITLB_MISSES].v.lval / 
   //                         performanceCtr[IC_MISSES].v.lval);
   // JEC: change definition to be over ALL fetches
   ITLB_P = ((double) 1.0 * performanceCtr[ITLB_MISSES].v.lval / 
                            total_instructions);
   
   memModel.initLatencies(TLB_LATENCY, L1_LATENCY, L2_LATENCY, MEM_LATENCY);
   memModel.initProbabilities(probLoadFromSTB,
                       1.0 - (performanceCtr[L1_MISSES].v.lval / 
                             (total_loads - totalLoadsFromSTB)),
                       1.0 - ((double)performanceCtr[L2_MISSES].v.lval /
                              performanceCtr[L1_MISSES].v.lval),
                       PT, 1.0 - i_miss, ic_p2, ITLB_P);

   double den;
   double a = 0.00000001;  // use a just to avoid divide-by-zero

   // Probablility of a load
   den = total_instructions - instructionProb[DELAY_SLOT_N].v.lval;
   PLD = ((double) 1.0 * (total_loads - instructionProb[D_LOADS].v.lval) / den);
   P_8 = ((double) 1.0 * (total_loads) / (total_instructions));
   iTypeProbCDF[I_LOAD] = ((double)(total_loads - instructionProb[D_LOADS].v.lval)) / 
                           (total_instructions - instructionProb[DELAY_SLOT_N].v.lval);
   iTypeDelaySlotProbCDF[I_LOAD] = ((double)(instructionProb[D_LOADS].v.lval)) / 
                                    (a + instructionProb[DELAY_SLOT_N].v.lval);

   // Probability of store if no load
   den = den - (total_loads - instructionProb[D_LOADS].v.lval);
   PST = ((double) 1.0 * (total_stores - instructionProb[D_STORES].v.lval) / den);
   iTypeProbCDF[I_STORE] = iTypeProbCDF[I_LOAD] + 
                           ((double)(total_stores - instructionProb[D_STORES].v.lval)) / 
                           (total_instructions - instructionProb[DELAY_SLOT_N].v.lval);
   iTypeDelaySlotProbCDF[I_STORE] = iTypeDelaySlotProbCDF[I_LOAD] + 
                           ((double)(instructionProb[D_STORES].v.lval)) / 
                            (a + instructionProb[DELAY_SLOT_N].v.lval);

   // Probability of a branch GIVEN no load/store 
   den = den - (total_stores - instructionProb[D_STORES].v.lval);
   PBR = ((double) 1.0 * (total_br) / den);
   P_3 = ((double) 1.0 * (total_br) / (total_instructions));
   iTypeProbCDF[I_BRANCH] = iTypeProbCDF[I_STORE] + 
                           ((double)(total_br)) / 
                           (total_instructions - instructionProb[DELAY_SLOT_N].v.lval);
   iTypeDelaySlotProbCDF[I_BRANCH] = iTypeDelaySlotProbCDF[I_STORE] + 
                           0.0; // branch in delay slot is impossible

   // If not a load/store/branch token, what is the probability of a GR producer 
   den = den - total_br;
   PG = ((double) 1.0 * (total_int - instructionProb[D_INTS].v.lval) / den);
   P_7 = ((double) 1.0 * (instructionProb[PB_6_FGU_N].v.lval +
                          instructionProb[PB_30_FGU_N].v.lval +
                          instructionProb[PB_6_INT_N].v.lval + 
                          instructionProb[PB_25_INT_N].v.lval + 
                          instructionProb[PB_6_FGU_D_N].v.lval + 
                          instructionProb[PB_30_FGU_D_N].v.lval + 
                          instructionProb[PB_6_INT_D_N].v.lval +
                          instructionProb[PB_25_INT_D_N].v.lval) / (total_instructions));
   iTypeProbCDF[I_GRPROD] = iTypeProbCDF[I_BRANCH] + 
                           ((double)(total_int - instructionProb[D_INTS].v.lval)) / 
                           (total_instructions - instructionProb[DELAY_SLOT_N].v.lval);
   iTypeDelaySlotProbCDF[I_GRPROD] = iTypeDelaySlotProbCDF[I_BRANCH] + 
                           ((double)(instructionProb[D_INTS].v.lval)) / 
                           (a + instructionProb[DELAY_SLOT_N].v.lval);

   // If not a load/store/branch/GR-producing token, what is the probability of a FR producer
   den = den - (total_int - instructionProb[D_INTS].v.lval);
   PF = ((double) 1.0 * (total_fp - instructionProb[D_FLOATS].v.lval) / den);
   P_4 = ((double) 1.0 * total_fp / (total_instructions));
   P_5 = (double) 1.0 * (instructionProb[P_FDIV_FSQRT_S_N].v.lval +
                         instructionProb[P_FDIV_FSQRT_D_N].v.lval +
                         instructionProb[P_FDIV_FSQRT_S_D_N].v.lval + 
                         instructionProb[P_FDIV_FSQRT_D_D_N].v.lval) / (total_instructions);
   P_6 = (double) 1.0 * (total_fp - (instructionProb[P_FDIV_FSQRT_S_N].v.lval + 
                                     instructionProb[P_FDIV_FSQRT_D_N].v.lval +
                                     instructionProb[P_FDIV_FSQRT_S_D_N].v.lval + 
                                     instructionProb[P_FDIV_FSQRT_D_D_N].v.lval)) /
                           (total_instructions);
   iTypeProbCDF[I_FLOAT] = iTypeProbCDF[I_GRPROD] + 
                           ((double)(total_fp - instructionProb[D_FLOATS].v.lval)) / 
                           (total_instructions - instructionProb[DELAY_SLOT_N].v.lval);
   iTypeDelaySlotProbCDF[I_FLOAT] = iTypeDelaySlotProbCDF[I_GRPROD] + 
                           ((double)(instructionProb[D_FLOATS].v.lval)) / 
                            (a + instructionProb[DELAY_SLOT_N].v.lval);
                           
   if (Debug) fprintf(stderr,"iProbCDF: %g %g %g %g %g\n", iTypeProbCDF[I_LOAD],
           iTypeProbCDF[I_STORE], iTypeProbCDF[I_BRANCH],
           iTypeProbCDF[I_GRPROD], iTypeProbCDF[I_FLOAT]);
   if (Debug) fprintf(stderr,"iDSProbCDF: %g %g %g %g %g\n", iTypeDelaySlotProbCDF[I_LOAD],
           iTypeDelaySlotProbCDF[I_STORE], iTypeDelaySlotProbCDF[I_BRANCH],
           iTypeDelaySlotProbCDF[I_GRPROD], iTypeDelaySlotProbCDF[I_FLOAT]);

   //
   // JEC: Initialize arrays for sub-category CDF probabilities, and
   // copies for delay slot as well
   //
   
   den = total_loads - (double) instructionProb[D_LOADS].v.lval;
   loadCatProbCDF[PB_6_MEM] = instructionProb[PB_6_MEM_N].v.lval / den;
   loadCatProbCDF[PB_25_MEM] = loadCatProbCDF[PB_6_MEM] + instructionProb[PB_25_MEM_N].v.lval / den;
   loadCatProbCDF[PB_3_LD] = loadCatProbCDF[PB_25_MEM] + instructionProb[PB_3_LD_N].v.lval / den;
   loadCatProbCDF[OTHER_LD] = 1.0;
   if (Debug) fprintf(stderr,"loadProbCDF: %g %g %g %g\n", loadCatProbCDF[PB_6_MEM],
           loadCatProbCDF[PB_25_MEM], loadCatProbCDF[PB_3_LD], 
           loadCatProbCDF[OTHER_LD]);

   den = total_int - (double) instructionProb[D_INTS].v.lval;
   intCatProbCDF[PB_6_FGU] = instructionProb[PB_6_FGU_N].v.lval / den;
   intCatProbCDF[PB_30_FGU] = intCatProbCDF[PB_6_FGU] + instructionProb[PB_30_FGU_N].v.lval / den;
   intCatProbCDF[PB_6_INT] = intCatProbCDF[PB_30_FGU] + instructionProb[PB_6_INT_N].v.lval / den;
   intCatProbCDF[PB_25_INT] = intCatProbCDF[PB_6_INT] + instructionProb[PB_25_INT_N].v.lval / den;
   intCatProbCDF[OTHER_INT] = 1.0;
   if (Debug) fprintf(stderr,"intProbCDF: %g %g %g %g %g\n", intCatProbCDF[PB_6_FGU],
           intCatProbCDF[PB_30_FGU], intCatProbCDF[PB_6_INT],
           intCatProbCDF[PB_25_INT], intCatProbCDF[OTHER_INT]);

   den = total_fp - (double) instructionProb[D_FLOATS].v.lval;
   floatCatProbCDF[FDIV_FSQRT_S] = instructionProb[P_FDIV_FSQRT_S_N].v.lval / den;
   floatCatProbCDF[FDIV_FSQRT_D] = floatCatProbCDF[FDIV_FSQRT_S] + 
                                     instructionProb[P_FDIV_FSQRT_D_N].v.lval / den;
   floatCatProbCDF[OTHER_FLOAT] = 1.0;
   if (Debug) fprintf(stderr,"floatProbCDF: %g %g %g\n", floatCatProbCDF[FDIV_FSQRT_S],
           floatCatProbCDF[FDIV_FSQRT_D], floatCatProbCDF[OTHER_FLOAT]);

   den = 0.00001 + instructionProb[D_LOADS].v.lval;
   loadCatDelaySlotProbCDF[PB_6_MEM] = instructionProb[PB_6_MEM_D_N].v.lval / den;
   loadCatDelaySlotProbCDF[PB_25_MEM] = loadCatDelaySlotProbCDF[PB_6_MEM] + 
                                        instructionProb[PB_25_MEM_D_N].v.lval / den;
   loadCatDelaySlotProbCDF[PB_3_LD] = loadCatDelaySlotProbCDF[PB_25_MEM] +
                                      instructionProb[PB_3_LD_D_N].v.lval / den;
   loadCatDelaySlotProbCDF[OTHER_LD] = 1.0;
   if (Debug) fprintf(stderr,"loadDSProbCDF: %g %g %g %g\n", 
           loadCatDelaySlotProbCDF[PB_6_MEM], loadCatDelaySlotProbCDF[PB_25_MEM],
           loadCatDelaySlotProbCDF[PB_3_LD], loadCatDelaySlotProbCDF[OTHER_LD]);

   den = 0.00001 + instructionProb[D_INTS].v.lval;
   intCatDelaySlotProbCDF[PB_6_FGU] = instructionProb[PB_6_FGU_D_N].v.lval / den;
   intCatDelaySlotProbCDF[PB_30_FGU] = intCatDelaySlotProbCDF[PB_6_FGU] +
                                       instructionProb[PB_30_FGU_D_N].v.lval / den;
   intCatDelaySlotProbCDF[PB_6_INT] = intCatDelaySlotProbCDF[PB_30_FGU] + 
                                      instructionProb[PB_6_INT_D_N].v.lval / den;
   intCatDelaySlotProbCDF[PB_25_INT] = intCatDelaySlotProbCDF[PB_6_INT] +
                                       instructionProb[PB_25_INT_D_N].v.lval / den;
   intCatDelaySlotProbCDF[OTHER_INT] = 1.0;
   if (Debug) fprintf(stderr,"intDSProbCDF: %g %g %g %g %g\n",
           intCatDelaySlotProbCDF[PB_6_FGU], intCatDelaySlotProbCDF[PB_30_FGU],
           intCatDelaySlotProbCDF[PB_6_INT], intCatDelaySlotProbCDF[PB_25_INT],
           intCatDelaySlotProbCDF[OTHER_INT]);

   den = 0.00001 + instructionProb[D_FLOATS].v.lval;
   floatCatDelaySlotProbCDF[FDIV_FSQRT_S] = instructionProb[P_FDIV_FSQRT_S_D_N].v.lval / den;
   floatCatDelaySlotProbCDF[FDIV_FSQRT_D] = 
                  floatCatDelaySlotProbCDF[FDIV_FSQRT_S] + 
                  instructionProb[P_FDIV_FSQRT_D_D_N].v.lval / den;
   floatCatDelaySlotProbCDF[OTHER_FLOAT] = 1.0;
   if (Debug) fprintf(stderr,"floatDSProbCDF: %g %g %g\n", 
           floatCatDelaySlotProbCDF[FDIV_FSQRT_S],
           floatCatDelaySlotProbCDF[FDIV_FSQRT_D],
           floatCatDelaySlotProbCDF[OTHER_FLOAT]);
    
   // Other Probabilities  
   // Probability of a branch miss GIVEN A BRANCH INST -- 
   // (fFrom Performance Counters)
   PBM = ((double) 1.0 * (performanceCtr[TAKEN_BRS].v.lval) / 
                          performanceCtr[TOTAL_BRS].v.lval);
   // Probability of an instruction that retires immediately/not retirre immediately
   P_1 = P_3 + P_4 + P_7 + P_8;
   P_2 = 1 - P_1;

   // Delay Slot Probabilities

   // JEC: leftover calculated values, not used anymore really
   PLD_D = (double) instructionProb[D_LOADS].v.lval / 
           (a + instructionProb[DELAY_SLOT_N].v.lval);
   PST_D = (double) instructionProb[D_STORES].v.lval / 
           (a + instructionProb[DELAY_SLOT_N].v.lval - 
            instructionProb[D_LOADS].v.lval);
   PG_D = (double) instructionProb[D_INTS].v.lval / 
          (a + instructionProb[DELAY_SLOT_N].v.lval -
           instructionProb[D_STORES].v.lval - instructionProb[D_LOADS].v.lval);
   PF_D = (double) instructionProb[D_FLOATS].v.lval / 
          (a + instructionProb[DELAY_SLOT_N].v.lval - 
           instructionProb[D_STORES].v.lval - instructionProb[D_LOADS].v.lval -
           instructionProb[D_INTS].v.lval);

   //display_constants();

   // Initalize Random number genrator
   if (seed == 0)
      seed = (unsigned long) (time(NULL) % 1000);
   init_genrand(seed);  // this function is defined in mersene.h
   printf("\nRandom Number Generator SEED initialized to %lu\n", seed);

   if (tracefile) {
      tracef = fopen(tracefile,"r");
      if (!tracef)
         fprintf(stderr,"Error opening trace file (%s); running MC...\n", tracefile);
   }
}


//
// JEC: Describe this here
//
void McNiagara::un_init(void)
{
   return;
}

///
/// @brief Finalize a model execution
///
/// @param outfile is the name of the output file
/// @return always 0
///
int McNiagara::fini(const char *outfile)
{
   FILE *outf;

   if (tracef)
      fclose(tracef);

   if (outfile) {
      outf = fopen(outfile, "w");
      if (outf == NULL) {
         fprintf(stderr, "Error opening output file %s\n", outfile);
         outf = stderr;
      }
   } else {
      outf = stdout;
   }

   fprintf(outf, "Latency ::\n");
   fprintf(outf, " L1    = %3d cycles\n", L1_LATENCY);
   fprintf(outf, " L2    = %3d cycles\n", L2_LATENCY);
   fprintf(outf, " Mem   = %3.0f cycles\n", (double) MEM_LATENCY);
   fprintf(outf, " TLB   = %3d cycles\n\n", TLB_LATENCY);

   fprintf(outf, "Model Diagaram Probabilities ::\n");
   fprintf(outf,
         " P1(Instr Not Retire Immed.)  = %f (%4.2f%% of instructions)\n",
         P_1, 100.0 * P_1);
   fprintf(outf,
         " P2(Instr Retires Immed.)     = %f (%4.2f%% of instructions)\n",
         P_2, 100.0 * P_2);
   fprintf(outf,
         " P3(Branch Instr)             = %f (%4.2f%% of instructions)\n",
         P_3, 100.0 * P_3);
   fprintf(outf,
         " P4(FP instr)                 = %f (%4.2f%% of instructions)\n",
         P_4, 100.0 * P_4);
   fprintf(outf,
         " P5(Fdiv/FSQRT Instr)         = %f (%4.2f%% of instructions)\n",
         P_5, 100.0 * P_5);
   fprintf(outf,
         " P6(All Other FGU Instr)      = %f (%4.2f%% of instructions)\n",
         P_6, 100.0 * P_6);
   fprintf(outf,
         " P7(SP. INT)                  = %f (%4.2f%% of instructions)\n",
         P_7, 100.0 * P_7);
   fprintf(outf,
         " P8(LOAD Instr)               = %f (%4.2f%% of instructions)\n",
         P_8, 100.0 * P_8);
   fprintf(outf,
         " P9(LOAD &  hit DTLB)         = %f (%4.2f%% of instructions)\n",
         P_9, 100.0 * P_9);
   fprintf(outf,
         " P10(LOAD & miss DTLB         = %f (%4.2f%% of instructions)\n",
         P_10, 100.0 * P_10);
   fprintf(outf,
         " P11(LOAD & hit L1)           = %f (%4.2f%% of instructions)\n",
         P_11, 100.0 * P_11);
   fprintf(outf,
         " P12(LOAD & hit L2)           = %f (%4.2f%% of instructions)\n",
         P_12, 100.0 * P_12);
   fprintf(outf,
         " P13(LOAD & hit MEM)          = %f (%4.2f%% of instructions)\n",
         P_13, 100.0 * P_13);

   fprintf(outf, "\nOther Useful Probabilities used internally ::\n");
   fprintf(outf, " P(Branch mispredict)                   =  %f (%4.2f%%)\n",
         PBM, 100.0 * PBM);
   fprintf(outf, " P(inst in delay slot executed)         =  %f (%4.2f%%)\n",
         instructionProb[P_DS].v.dval, 100.0 * instructionProb[P_DS].v.dval);
   fprintf(outf, " P(LD)                                  =  %f (%4.2f%%)\n",
         PLD, 100.0 * PLD);
   fprintf(outf, " P(ST| no load)                         =  %f (%4.2f%%)\n",
         PST, 100.0 * PST);
   fprintf(outf, " P(BR| no load/st)                      =  %f (%4.2f%%)\n",
         PBR, 100.0 * PBR);
   fprintf(outf, " P(INT|no load/st/br)                   =  %f (%4.2f%%)\n",
         PG, 100.0 * PG);
   fprintf(outf, " P(FP| no load/st/br/int)               =  %f (%4.2f%%)\n",
         PF, 100.0 * PF);
   fprintf(outf, " P(LD in delay slot)                    =  %f (%4.2f%%)\n",
         PLD_D, 100.0 * PLD_D);
   fprintf(outf, " P(ST| no load in delay slot)           =  %f (%4.2f%%)\n",
         PST_D, 100.0 * PST_D);
   fprintf(outf, " P(INT|no load/st/br in delay slot)     =  %f (%4.2f%%)\n",
         PG_D, 100.0 * PG_D);
   fprintf(outf, " P(FP| no load/st/br/int in delay slot) =  %f (%4.2f%%)\n",
         PF_D, 100.0 * PF_D);

   fprintf(outf, "\nTotal Instructions simulated: %llu (delay slot %llu)\n\n", 
           tot_insns, tot_delayslot_insns);
   fprintf(outf, "Mem Ops      = %10llu ( %4.2f%% of all tokens )\n",
         n_memops, 100.0 * n_memops / tot_insns);
   fprintf(outf, "Loads        = %10llu ( %4.2f%% of all tokens )\n", n_loads,
         100.0 * n_loads / tot_insns);
   fprintf(outf, "Stores       = %10llu ( %4.2f%% of all tokens )\n",
         n_stores, 100.0 * n_stores / tot_insns);
   fprintf(outf, "Branches     = %10llu ( %4.2f%% of all tokens )\n",
         n_branches, 100.0 * n_branches / tot_insns);
   fprintf(outf, "FR producers = %10llu ( %4.2f%% of all tokens )\n",
         n_fr_produced, 100.0 * n_fr_produced / tot_insns);
   fprintf(outf, "GR producers = %10llu ( %4.2f%% of all tokens )\n",
         n_gr_produced, 100.0 * n_gr_produced / tot_insns);

   unsigned long long mm_numloads, mm_numiloads, mm_num_ichits,
                      mm_numil2hits, mm_numimemhits, mm_numitlb, mm_numstores;
   memModel.getDataLoadStats(&mm_numloads, &n_stb_reads, &n_l1,
                             &n_l2, &n_mem, &n_tlb);
   memModel.getInstLoadStats(&mm_numiloads, &mm_num_ichits, &mm_numil2hits,
                             &mm_numimemhits, &mm_numitlb);
   memModel.getStoreStats(&mm_numstores);
   
   fprintf(outf, "\nMM total loads: %llu\n", mm_numloads);
   fprintf(outf, "Loads from ST Buffer = %10llu (%4.2f%% of loads   )\n",
         n_stb_reads, 100.0 * n_stb_reads / n_loads);
   fprintf(outf,
         "Loads to L1  = %10llu (%4.2f%% of loads, %4.2f%% of all tokens)\n",
         n_l1, 100.0 * n_l1 / n_loads, 100.0 * n_l1 / tot_insns);
   fprintf(outf,
         "Loads to L2  = %10llu (%4.2f%% of loads, %4.2f%% of all tokens)\n",
         n_l2, 100.0 * n_l2 / n_loads, 100.0 * n_l2 / tot_insns);
   fprintf(outf,
         "Loads to Mem = %10llu (%4.2f%% of loads, %4.2f%% of all tokens)\n",
         n_mem, 100.0 * n_mem / n_loads, 100.0 * n_mem / tot_insns);
   fprintf(outf, "DTLB miss    = %10llu (%4.2f%% of loads )\n", n_tlb,
         100.0 * n_tlb / n_loads);
   fprintf(outf, "ITLB miss    = %10llu (%4.2f%% of insns )\n", mm_numitlb,
         100.0 *  mm_numitlb / tot_insns);
   fprintf(outf, "I$ misses    = %10llu (%4.2f%% of tokens)\n",
         n_icache_misses, 100.0 * n_icache_misses / tot_insns);

   fprintf(outf, "\n");
   fprintf(outf, "Pipeline Flushes     = %10llu (%4.2f%% of tokens  )\n",
         n_pipe_flushes, 100.0 * n_pipe_flushes / tot_insns);
   fprintf(outf, "ST Buffer Full Stalls= %10llu (%4.2f%% of stores  )\n",
         n_stb_full, 100.0 * n_stb_full / (n_stores));

   fprintf(outf, "\n");
   
   double total_cpi;
   unsigned int r;
   fprintf(outf, "CPI Components:\n");
   //fprintf(outf, "Reasons for cycle accumulation:\n");
   for (r=0; r < CycleTracker::NUMCYCLEREASONS; r++) {
      fprintf(outf, "%13.13s = %4.5f  (%4.02f%% of CPI)\n",
              cycleTracker.categoryName((CycleTracker::CycleReason)r),
              cycleTracker.cyclesForCategory((CycleTracker::CycleReason)r) / tot_insns,
              cycleTracker.cyclePercentForCategory((CycleTracker::CycleReason)r));
   }
   total_cpi = cycleTracker.currentCycles() / tot_insns;
   fprintf(outf,"%13.13s = %4.5f  (%4.02f%% of CPI)\n", "TOTAL",
           total_cpi, 100.0);

   fprintf(outf, "\nCompare To REAL MEASUREMENTS Below:");
   fprintf(outf, "\nLD  = %4.2f", (double) performanceCtr[LD_PERC].v.dval);
   fprintf(outf, "\nST  = %4.2f", (double) performanceCtr[ST_PERC].v.dval);
   fprintf(outf, "\nBR  = %4.2f", (double) performanceCtr[BR_PERC].v.dval);
   fprintf(outf, "\nFP  = %4.2f", (double) performanceCtr[FP_PERC].v.dval);
   fprintf(outf, "\nGR  = %4.2f", (double) performanceCtr[GR_PERC].v.dval);

   fprintf(outf, "\n\nResults Summary: ");
   fprintf(outf, "\nTotal instructions executed = %llu\n", tot_insns);
   fprintf(outf, "\nMeasured CPI  = %4.5f", performanceCtr[MEASURED_CPI].v.dval);
   fprintf(outf, "\nPredicted CPI = %4.5f", total_cpi);
   fprintf(outf, "\nDifference    = %4.5f%%\n\n",
         100 * (total_cpi - performanceCtr[MEASURED_CPI].v.dval) / 
               performanceCtr[MEASURED_CPI].v.dval);
   fclose(outf);
   un_init();
   return 0;
}

#ifndef SST
/// Dummy (empty) off-CPU interface class
class NullIF: public OffCpuIF
{
  public:
     void  memoryAccess(access_mode mode, unsigned long long address, 
                        unsigned long data_size)
     {
        //fprintf(stderr, "Memory access: mode %d addr %llx size %ld\n",
        //        mode, address, data_size);
     }
     virtual void  NICAccess(access_mode mode, unsigned long data_size)
     {
        //fprintf(stderr, "NIC access: mode %d size %ld\n",
        //        mode, data_size);
     }
};

static const char* helpMessage = 
"\nUsage: mcniagara [options]\n\
Options:\n\
 --seed #          set random number seed\n\
                   (default: based on time())\n\
 --ihist filename  use named file for histogram input file\n\
                   (default: INPUT)\n\
 --iprob filename  use named file for instruction probabilities\n\
                   (default: inst_prob.h)\n\
 --perf  filename  use named file for performance counter data\n\
                   (default: perf_cnt.h)\n\
 --trace filename  use named file for trace-drive simulation\n\
                   (default: perform stochastic simulation)\n\
 --outf  filename  use named file for output results\n\
                   (default: print to stdout)\n\
\n";

void doHelp()
{
   fputs(helpMessage, stderr);
   exit(1);
}

///
/// @brief run a MC/trace model execution
/// 
/// For a trace-driven, execution, provide the trace file
/// name as the only command-line argument
///
int main(int argc, char **argv)
{
   int i,j;
   NullIF extIf;
   McNiagara cpu;
   const char *inputfile = "INPUT";
   char *outputfile = 0;
   const char *iprobfile = "inst_prob.h";
   const char *pcntfile = "perf_cnt.h";
   char *tracefile = 0;    // default: run monte carlo
   unsigned long seed = 0; // default: select time-random seed
   i = 1;
   if (argc == 2) {
      doHelp();
      return 1;
   }
   while (i+1 < argc) {
      if (!strcmp(argv[i],"--seed"))
         seed = strtoul(argv[++i],0,10);
      else if (!strcmp(argv[i],"--ihist"))
         inputfile = argv[++i];
      else if (!strcmp(argv[i],"--iprob"))
         iprobfile = argv[++i];
      else if (!strcmp(argv[i],"--perf"))
         pcntfile = argv[++i];
      else if (!strcmp(argv[i],"--trace"))
         tracefile = argv[++i];
      else {
         doHelp();
         return 1;
      }
      i++;
   }
   cpu.init(inputfile, &extIf, iprobfile, pcntfile, tracefile, seed);
   for (i=0; i<999999; i++) {
      for (j=0; j < 10000; j++)
         if (cpu.sim_cycle(i)) break;
      if (cpu.convergence || cpu.traceEnded) break;
   }
   cpu.fini(outputfile);
   return 0;
}
#endif
