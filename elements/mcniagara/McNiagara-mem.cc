//
// THIS FILE IS NOT USED, DO NOT UPDATE IT ANYMORE (JEC)
//
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "McNiagara.h"
//#include "perf_cnt.h"           // Performance Counters data!
//#include "inst_prob.h"          // Instruction Special Probabilities

///
/// @brief Add main memory request to memory queue
///
/// This adds stores and loads to the memory queue;
/// for loads, when_satisfied is the cycle count in the future 
/// when the load will be done;
/// for stores, when_satisfied is how much long (in cycles) the 
/// store willstay in the store buffer (STB)
/// @param when_satisfied is the duration cycle count described above
/// @param whoami is the memory op type (load or store)
///
/***
void McNiagara::add_memq(double when_satisfied, enum token_type whoami)
{
   struct memory_queue *temp,
               *temp2;

   if (whoami == IS_LOAD)
      ld_in_q++;
   else
      st_in_q++;

   temp = (struct memory_queue *) malloc(sizeof(struct memory_queue));
   temp2 = (struct memory_queue *) malloc(sizeof(struct memory_queue));

   assert(temp);
   if (whoami == IS_STORE) {
      // if there has been a store before
      //
      // JEC: This will only ever record the FIRST store in the
      // variable last_store, and never updates it;
      //
      if (last_store) {
         if (cycles >= last_store->when_satisfied)
            when_satisfied += cycles;
         else
            when_satisfied += last_store->when_satisfied;
      } else
         // first store in the instruction stream
      {
         when_satisfied += cycles;
         temp2->when_satisfied = when_satisfied;
         temp2->whoami = whoami;
         temp2->next = NULL;
         last_store = temp2;

      }
      // now we add the mem request to the mem q
      temp->when_satisfied = when_satisfied;
      temp->whoami = whoami;
      temp->next = mq_head;
      mq_head = temp;

   }
   // if load
   else {
      temp->when_satisfied = when_satisfied;
      temp->whoami = whoami;
      temp->next = mq_head;
      mq_head = temp;
   }

}


///
/// @brief Scan memory queue and removes completed requests
///
/// @return always 0.0
///
double McNiagara::scan_memq(void)
{
   double ans = 0;
   struct memory_queue *temp,
               *next,
               *prev;

   // Scan the chain of memory queue tokens and remove completed ones
   for (prev = NULL, temp = mq_head; temp;) {
      // if completed....
      if (cycles >= temp->when_satisfied) {
         if (temp->whoami == IS_LOAD)
            ld_in_q--;
         else
            st_in_q--;

         if (prev)
            prev->next = temp->next;
         else
            mq_head = temp->next;

         next = temp->next;
         free(temp);
         temp = next;
      } else {
         prev = temp;
         temp = temp->next;
      }
   }

   // ans is always zero
   return ans;

}


///
/// @brief  Determine how long to satisfy this load
///
/// This simulates the memory model (and probabilities) to determine
/// how long a load will take to be satisfied. (JEC: should the TLB
/// cost be tracked separately rather than folded into the reason?)
/// @param where an out-parameter indicated where the load is satisfied from
/// @param flag is (???)
/// @param d_dist is (???)
/// @param l_dist is (???)
/// @return number of cycles needed to serve load
///
double McNiagara::cycles_to_serve_load(enum StallReason *where, int flag,
                                   int d_dist, int l_dist)
{
   enum StallReason reason;
   double c = 0.0;

   // L1 hit?
   if (my_rand() <= P1) {
      // TLB miss?
      if (my_rand() <= PT) {
         n_tlb++;
         c += TLB_LATENCY;
      }
      reason = L1_CACHE;
      n_l1++;
      if (d_dist == 0 || (d_dist + 1) % 4 == 0 || (l_dist + 1) % 4 == 0) {
         //stall for 3 cycles
         c += L1_LATENCY;
      } else if (d_dist == 1) {
         //stall for 2 cycles
         c += (L1_LATENCY - 1);
      } else {
         //don't stall 
         c += 0;
      }

   }
   // L2 hit?
   else if (my_rand() <= P2) {

      if (my_rand() <= PT) {
         n_tlb++;
         c = TLB_LATENCY;
      }
      reason = L2_CACHE;
      n_l2++;
      if (d_dist % 4 == 0 || l_dist == 2 || (l_dist - 2) % 4 == 0) {
         //stall for 24 cycles
         c += (L2_LATENCY + 1);

      } else if ((d_dist + 1) % 2 || l_dist == 1 || (l_dist - 1) % 4 == 0) {
         //stall for 22 cycles only
         c += (L2_LATENCY - 1);

      } else {
         //stall for 23 cycles
         c += (L2_LATENCY - 1);
      }

   }
   // Memory hit? better be!
   else if (my_rand() <= PM) {
      // TLB miss?
      if (my_rand() <= PT) {
         n_tlb++;
         c = TLB_LATENCY;
      }
      reason = MEMORY;
      n_mem++;
      c += MEM_LATENCY;

      c += scan_memq();  // this scans mem q and removes completed tokens 
   }

   else {
      assert(0);
   }

   *where = reason;
   return c;
}
***/

/***
///
/// @brief Add a dependency to the memory chain
///
/// This records a dependency for a future instruction, so that the
/// simulation knows to stall it if necessary
/// @param which_inst is the count of the (future) dependent instruction 
/// @param when_satisfied is the cycle when its dependency will be satisfied
/// @param reason is the memory unit responsible for providing value
///
void McNiagara::add_dependency(unsigned long long which_inst,
                              double when_satisfied,
                              enum StallReason reason)
{
   struct dependency *temp;

   // Scan the chain for the dependent instruction to see if it's
   // already there waiting 
   for (temp = dc_head; temp && temp->which_inst != which_inst;
         temp = temp->next);

   // Found. This dependent instruction exists in chain
   if (temp) {
      // New dependency is bigger (longer latency) than existing one
      if (when_satisfied > temp->when_satisfied) {
         temp->when_satisfied = when_satisfied;
         temp->reason = reason;
      }
      // Existing dependence is bigger than new one
      else {
      }
   }
   // Not found. Add this to the chain
   else {
      temp = (struct dependency *) malloc(sizeof(struct dependency));
      assert(temp);

      temp->which_inst = which_inst;
      temp->when_satisfied = when_satisfied;
      temp->reason = reason;
      temp->next = dc_head;
      dc_head = temp;
   }
}

///
/// @brief Adjust dependence chains after pipeline flush (?)
///
/// whenever we flush the pipe we need to adjust the dependence chain
/// For the most part this adjustment is actually counted for in the
/// latency of instructions such as loads that miss L1 and mispredicted branches
/// But, it's done mostly for i-cache misses.
/// NOTE-BY-JEC: The dependence chain should never be adjusted because the
/// producer of the value used in the dependence is AHEAD of any pipeline 
/// stall or flush, so that even though the consumer is delayed, the value
/// is still available when it originally was. 
///
void McNiagara::adjust_dependence_chain(double c)
{
   struct dependency *temp,
             *next;

   for (temp = dc_head; temp;) {
      temp->when_satisfied = temp->when_satisfied + c;
      next = temp->next;
      temp = next;
   }                            // end for

   //free(temp);
   //free(next);

}

///
/// Will return the cycle number at which the depndency for the current
/// instruction is satisfied
///
double McNiagara::is_dependent(unsigned long long which_inst, enum StallReason *where)
{
   struct dependency *temp,
             *prev;
   double ret_val = 0;

   // Scan the chain for the dependent instruction
   for (prev = NULL, temp = dc_head; temp && temp->which_inst != which_inst;
         prev = temp, temp = temp->next);

   // Found. Unlink node and return when_satisfied
   if (temp) {
      if (prev) {
         prev->next = temp->next;
      } else {
         dc_head = temp->next;
      }

      ret_val = temp->when_satisfied;
      *where = temp->reason;
      free(temp);
   }

   return ret_val;
}
***/

//
// JEC: Describe this here!
//
/***
void McNiagara::serve_store(void)
{
   int c = 0;

   c += (int) scan_memq();      // this scans mem q and removes compleled tokens and return extra latency if needed
   c = (int) (my_rand() * 100) % 8;     // this will give me a random number between 0 and 8
   add_memq(c, IS_STORE);       // when_satisfied is will be how much longer the store will stay in the STB 

}
***/
