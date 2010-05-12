#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "McNiagara.h"

//#include "inst_prob.h"   // Instruction Special Probabilities
// NOTE: the inst_prob defs are now read in as a data file
//       and accessed through the instructionProb[] array
// NOTE2: all of the commented out accesses have NOT been updated
//        to reflect this change

///
/// @brief Simulate one CPU cycle
///
/// Run the simulation for one cycle; with multiple issue
/// this will simulate multiple instructions
/// @param current_cycle is the current cycle count
/// @return always 0
///
int McNiagara::sim_cycle(unsigned long current_cycle)
{
   double d;
   Token token;

   // Simulate only for one cycle, if cpu issues
   // multiple instructions in one cycle, this loop
   // will handle them
   for (d = 0.0; d < 0.999; d += CPIi) {
      if (generate_instruction(&token)) return 1;
      sim_instruction(&token);
   }
   return 0;
}

///
/// @brief Generate an instruction token
/// 
/// This generates a probabilistic instruction token
/// in the Monte Carlo mode, or reads the next instruction
/// from a trace file in the trace-driven mode. It sets
/// all appropriate instruction parameters for the needs
/// of the simulation. 
/// @param token is a pointer to the token type to fill in
/// @return 0 in success, other on error
///
int McNiagara::generate_instruction(Token *token)
{
   if (!tracef) {

      // Monte Carlo instruction generation, driven by PRNG
      static bool lastInsnWasBranch = false;

      // default to use normal instruction probabilities
      double *useProbs = iTypeProbCDF;
      double p, dummyCat[1] = {1.0};
      double *catProbs = dummyCat;
      int i;

      // if last instruction was branch, set delay slot on prob
      token->inDelaySlot = false;
      if (lastInsnWasBranch) {
         if (my_rand() <= instructionProb[P_DS].v.dval) {
            token->inDelaySlot = true;
            useProbs = iTypeDelaySlotProbCDF; // use delay slot probs
         } else {
            // JEC: should generate a NOP?!?!?
            // yes - and mark it as in delay slot so mispredict costs
            // can be generated correctly
            // Q: should instruction count count a delay slot nop?
            token->inDelaySlot = true;
            token->type = I_NOP;
            return 0;
         }
      }
      lastInsnWasBranch = false;
      p =  my_rand();
      if (p <= useProbs[I_LOAD]) {
         token->type = I_LOAD;
         catProbs = (token->inDelaySlot)? loadCatDelaySlotProbCDF : loadCatProbCDF;
      } else if (p <= useProbs[I_STORE]) {
         token->type = I_STORE;
      } else if (p <= useProbs[I_BRANCH]) {
         token->type = I_BRANCH;
         lastInsnWasBranch = true;
      } else if (p <= useProbs[I_GRPROD]) {
         token->type = I_GRPROD;
         catProbs = (token->inDelaySlot)? intCatDelaySlotProbCDF : intCatProbCDF;
      } else if (p <= useProbs[I_FLOAT]) {
         token->type = I_FLOAT;
         catProbs = (token->inDelaySlot)? floatCatDelaySlotProbCDF : floatCatProbCDF;
      } else {
         fprintf(stderr, "MC Error: no instruction type selected!\n");
         return 1;
      }
      // set the category (inside each type) (also pass prob on)
      p = my_rand();
      token->optProb = p;
      for (i=0; catProbs[i] < 1.0; i++)
         if (p <= catProbs[i]) break;
      token->category.v = i;

      // end MC instruction token generation
      return 0;

   } else {

      // Trace driven simulation, read instruction info from trace file
      char line[128];
      int type, delay, cat;
      double p;
      if (!fgets(line, sizeof(line), tracef)) {
         if (Debug) fprintf(stderr, "EOF Error: trace file ended!\n");
         traceEnded = true;
         return 1;
      }
      if (sscanf(line,"%d%d%d", &type, &delay, &cat) != 3) {
         fprintf(stderr, "Error: couldn't read (%s)!\n", line);
         return 2;
      }
      if (Debug) fprintf(stderr,"read instruction: %d %d %d\n", type, delay, cat);
      switch (type) {
       case 1: token->type = I_LOAD; token->category.l = OTHER_LD; break;
       case 2: token->type = I_STORE; token->category.v = 0; break;
       case 3: token->type = I_BRANCH; token->category.v =0; break;
       case 4: token->type = I_FLOAT; token->category.f = OTHER_FLOAT; break;
       case 5: token->type = I_GRPROD; token->category.i = OTHER_INT; break;
       default: break;
      }
      switch (cat) {
       case 1: token->category.l = PB_6_MEM; break;
       case 2: token->category.l = PB_25_MEM; break;
       case 3: token->category.l = PB_3_LD; break;
       case 4: token->category.v = 0; break; // PB_6_CTI not used for now
       case 5: token->category.i = PB_25_INT; break;
       case 6: token->category.i = PB_30_FGU; break;
       case 7: token->category.i = PB_6_FGU; break;
       case 8: token->category.f = FDIV_FSQRT_S; break;
       case 9: token->category.f = FDIV_FSQRT_D; break;
       case 10: token->category.i = PB_6_INT; break;
       case 0: break; // do nothing, default "other" is already set
       default: break;
      }
      if (delay)
         token->inDelaySlot = true;
      else
         token->inDelaySlot = false;
      p = my_rand();
      token->optProb = p; // shouldn't be used but??
      return 0;
   } // end else trace driven
}

///
/// @brief Simulation one instruction
///
/// This simulates and accounts for one instruction as
/// represented by the token argument
/// @param token is the instruction token
/// @return 0 on success, other on error
///
int McNiagara::sim_instruction(Token *token)
{
   CycleCount when_satisfied; //, den;
   CycleCount cur_cpi = 0, latency;
   unsigned long long next_load, next_fp;
   int dep_distance;
   CycleTracker::CycleReason reason, where;
   CycleCount cycles2;
   
   // following vars need remembered in between instructions
   static CycleTracker::CycleReason last_ld_reason;
   static double last_cpi = 0.0;
   static double last_ld_satisfied = 0.0;
   static double delay_cycles; // cycles required by last branch

   // check if insn is in delay slot
   // JEC: does a delay slot insn really always take one cycle?
   if (token->inDelaySlot) {
      cycles += 1;           // Dispatching a delay slot requires a cycle
      cycleTracker.accountForCycles(1, CycleTracker::CPII);
      tot_delayslot_insns++; // Keep track of the number of instructions
                             // executed in the delay slot
      // JEC: delay slot insn is then processed normally? The main
      // instruction category probabilities were used correctly but
      // we still need to modify some internal probs I think
   } else {
      // is a regular instruction
      cycles += CPIi;
      cycleTracker.accountForCycles(CPIi, CycleTracker::CPII);
   }

   cycles2 = memModel.serveILoad(cycles-1, 0, 0, &reason);
   if (cycles2 > cycles+1.0001) {
      cycleTracker.accountForCycles(cycles2 - cycles, reason);
      n_icache_misses++;
      n_pipe_flushes++;
      cycles = cycles2;
   }
   
   // Is this instruction dependent on a previous instruction
   // JEC: this routine leaves where alone if no dependency found,
   // does that mean an old where value will be misused?
   when_satisfied = depTracker.isDependent(tot_insns, &where);

   // Yes. Not only is it dependent, we also need to STALL
   if (when_satisfied > cycles) {
         if (where == CycleTracker::INT_DSU_DEP)
            fprintf(stderr,"Recording INT_DSU_DEP: %g\n",
                    when_satisfied - cycles);
         if (where == CycleTracker::INT_USE_DEP)
            fprintf(stderr,"Recording INT_USE_DEP: %g\n",
                    when_satisfied - cycles);
      cycleTracker.accountForCycles(when_satisfied - cycles, where);
      cycles = when_satisfied;
   }
   // --- STALL check: done

   // is this a load?
   if (token->type == I_LOAD) {

      // Call out to external interface module for memory read
      if (my_rand() < 0.000001) {
         external_if->NICAccess(OffCpuIF::AM_READ, 7);
      } else {
         external_if->memoryAccess(OffCpuIF::AM_READ, 0x1000, 9);
      }

      n_memops++;
      n_loads++;
      cycles2 = cycles;

      // First ask memory model for cost of serving this load
      CycleCount satisfiedAt;
      satisfiedAt = memModel.serveLoad(cycles, 0, 0, &reason);
      cycleTracker.accountForCycles(satisfiedAt - cycles,
                                    reason);
      cycles = satisfiedAt;

      // loads missing L1 will cause a pipeline flush
      if (cycles - cycles2 >= L1_LATENCY) {
         n_pipe_flushes++;
      }

      // Then check for special categories of load-type instructions
      // - we think these latencies should be added on top of anything
      //   the memory hierarchy costs us
      if (token->category.l == PB_6_MEM) {
         latency = 6;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::SPCL_LOAD);
      } else if (token->category.l == PB_25_MEM) {
         latency = 25;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::SPCL_LOAD);
      } else if (token->category.l == PB_3_LD) {
         latency = 3;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::SPCL_LOAD);
      }

      // How far is the consumer?
      dep_distance = sample_hist(ld_use_hist, LD_USE_HIST_LENGTH);
      // How far is the next load?
      next_load = sample_hist(ld_ld_hist, LD_LD_HIST_LENGTH);

      // Add this to the dependency chain
      depTracker.addDependency(tot_insns + dep_distance, cycles, reason);

      last_ld_satisfied = cycles;
      last_ld_reason = reason;

   } else if (token->type == I_STORE) {

      n_memops++;
      n_stores++;

      // Call out to external interface module for memory read
      if (my_rand() < 0.000001) {
         external_if->NICAccess(OffCpuIF::AM_WRITE, 8);
      } else {
         external_if->memoryAccess(OffCpuIF::AM_WRITE, 0x4000, 8);
      }

      double satisfiedAt;
      satisfiedAt = memModel.serveStore(cycles, 0, 0, &reason);
      cycleTracker.accountForCycles(satisfiedAt - cycles, reason);

   } else if (token->type == I_BRANCH) {

      n_branches++;
      cycles2 = cycles + BRANCH_MISS_PENALTY;

      // Is this branch Mis-predicted? 
      // Note: Whenever a branch is mis-predicted a pipeline flush occurs
      if (token->optProb <= PBM) {
         delay_cycles = cycles + BRANCH_MISS_PENALTY;
         n_miss_branches++;
         n_pipe_flushes++;
      } else {
         // JEC addition: reset delay cycles back to current
         // -- is correct or not?
         delay_cycles = cycles; 
      }
      // See Page 74, if another DCTI (branch) is detected following this one?
      if (sample_hist(br_br_hist, BR_BR_HIST_LENGTH) <= 3) {
         //stall at least till the current branch finishes
         cycleTracker.accountForCycles(1, CycleTracker::BRANCH_ST);
         cycles += 1;
         delay_cycles += 1; // make sure delay slot cost stays same (JEC)
      }

   } else if (token->type == I_GRPROD) {

      n_gr_produced++;
      cycles2 = cycles;

      // Checking to see if this is a Special int instruction that stall the pipe
      // If this is an integer instruction that is executed by the FGU
      // If this is an integer multiply or popc or  pdist instructions
      if (token->category.i == PB_6_FGU) {
         latency = 8;           // micro-benchmarks say it's 8-10 cycles
         reason = CycleTracker::INT_DEP;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::INT_DEP);
      }
      // If this is an integer divide instruction
      else if (token->category.i == PB_30_FGU) {
         latency = 30;
         reason = CycleTracker::INT_DEP;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::INT_DEP);
      }
      // Now we check if this is a special INT instruction that stalls the pipe
      // These instructions are save, saved, restore, restored
      else if (token->category.i == PB_6_INT) {
         latency = 7;           // 6 in documentation 7 in microbenchmarks
         reason = CycleTracker::INT_DEP;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::INT_DEP);
      } 
      else if (token->category.i == PB_25_INT) {
         latency = 25;
         reason = CycleTracker::INT_DEP;
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::INT_DEP);
      } else {
         latency = 0; // Latency of Simple INT instructions is reflected in CPIi
      }

      // how far is the consumer?
      dep_distance = sample_hist(int_use_hist, INT_USE_HIST_LENGTH);
      //fprintf(stderr,"dependence distance: %d\n", dep_distance);

      // Add this to the dependency chain
      if (token->inDelaySlot)
         depTracker.addDependency(tot_insns + dep_distance, cycles2 + latency, 
                                  CycleTracker::INT_DSU_DEP);
      else
         depTracker.addDependency(tot_insns + dep_distance, cycles2 + latency, 
                                  CycleTracker::INT_USE_DEP);
   }
   // Now it must be an FP instruction
   // NOTE: using micro-benchmarks, it turns out that fdiv and fsqrt are blocking
   // that is, they will use all of their latency before allowing any instruction
   // to go thru the pipe. This is unlike what the documentation says ! 
   else if (token->type == I_FLOAT) {

      n_fr_produced++;

      cycles2 = cycles;
      // how far is the consumer?
      dep_distance = sample_hist(fp_use_hist, FP_USE_HIST_LENGTH);
      // how far is the next fp instruction?
      next_fp = sample_hist(fp_fp_hist, FP_FP_HIST_LENGTH);

      if (token->category.f == FDIV_FSQRT_S) {
         if (last_fdiv == (tot_insns - 1))
            latency = 23;       // 23 fdiv_followed by fdiv
         else if (next_fp == 1 || dep_distance == 1)
            latency = 22;       // 22
         else
            latency = 21;       // 21
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::FGU_DEP);
         last_fdiv = tot_insns;
      } else if (token->category.f == FDIV_FSQRT_D) {
         if (last_fdiv == (tot_insns - 1))
            latency = 37;       // 37
         else if (dep_distance == 1)
            latency = 36;       //36
         else
            latency = 35;       // 35
         cycles += latency;     // stall
         cycleTracker.accountForCycles(latency, CycleTracker::FGU_DEP);
         last_fdiv = tot_insns;
      }
      // All other FGU instructions
      else {
         if (dep_distance <= 4 && next_fp <= 4) {
            latency = FGU_LATENCY - 2;  // 5 cycles ==> forwarding
            cycles += latency;  // stall
            cycleTracker.accountForCycles(latency, CycleTracker::FGU_DEP);
         } else
            latency = 0;        // will take 1 cycle which is reflected in CPIi 

      }

      // Add this to the dependency chain
      depTracker.addDependency(tot_insns + dep_distance, cycles2 + latency,
                               CycleTracker::FGU_DEP);

   } else if (token->type == I_NOP) {
      // do nothing here, but cycles will increase by CPIi and
      // the delay slot check below will occur
      // fprintf(stderr,"Did NOP!\n");   
   } else {
      fprintf(stderr, "Error: token type was unknown!\n");
   }
   
   // Now handle delay slot case, if instruction was in delay slot
   // but did not use enough cycles to account for branch 
   // JEC: rework logic to allow for a minimum mispredict cost
   // of three cycles, because of instructions already in pipeline
   if (token->inDelaySlot) {
      double br_cycles;
      br_cycles =  delay_cycles - cycles;
      if (br_cycles < 3)
         br_cycles = 3;
      cycleTracker.accountForCycles(br_cycles, CycleTracker::BRANCH_MP);
      cycles += br_cycles;  // stall
   }

   tot_insns++;

   // termination check -- just mark convergence flag
   //
   if(tot_insns % 1000000 == 0) 
   {
      //printf("."); fflush(stdout);
      cur_cpi = (double) cycles / tot_insns;
      // Is this an early finish? 
      if(fabs(cur_cpi - last_cpi) < THRESHOLD)
      {
         convergence = true;
      }
      last_cpi = cur_cpi;
   }

   return 0;
}
