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


/*---------------------------------------------------------+-----------------\
|  Cache-only Timing Model                                 | Chad D. Kersey  |
+----------------------------------------------------------+-----------------+
|  Simple timing model that implements a configurable multi-level cache. The |
|  CPU core is assumed to progress at 1 instruction per cycle outside of     |
|  cache misses. All caches are modeled as physically indexed and tagged.    |
|  DMA accesses are not modeled.                                             |
\---------------------------------------------------------------------------*/
#include <cstdio>
#include <cstdlib>
#include <new>
#include "model.h"

/* Module factory - standard boilerplate code. */
extern "C" {
  Models::Model *mod_obj = NULL;
  void __init(void);
}

/* Could just say "using namespace Models" but this way we are only "using" 
   what we need. */
using Models::Model;            using Models::CallbackHandler;
using Models::MemOpHandler;     using Models::MemAccessType;
using Models::InstructionHandler;

using Models::callbackRequest;  using Models::getCycle;
using Models::memRead;          using Models::memWrite;
using Models::setIpc;           using Models::ParamIt;
using Models::model_params;

class CacheModel: 
  public Model,              // Required inheritance for all models.
  public CallbackHandler,    // Receives callback events.
  public MemOpHandler,       // Receives memory operation events.
  public InstructionHandler  // Receives instruction events.
{
private:

  struct TagArray {
    uint32_t linesPerWay, ways; 
    uint32_t *tags; // Indexed by [way * lines_per_way + line]
    bool *valid;    // Also indexed by [way * lines_per_way + line]
    TagArray(uint32_t linesPerWay, uint32_t ways) :
      linesPerWay(linesPerWay), ways(ways) {
      tags = new uint32_t[linesPerWay * ways];
      valid = new bool[linesPerWay * ways];
      for (int i = 0; i < linesPerWay * ways; i++) valid[i] = false;
    }
    ~TagArray() { delete[] tags; }
    bool findTag(uint32_t index, uint32_t tag) {
      for (int i = 0; i < ways; i++) {
        uint32_t j = linesPerWay*i + index;
	if (valid[j] && tags[j] == tag) return true;
      }
      return false;
    }
    void setTag(uint32_t way, uint32_t index, uint32_t tag) {
      tags[linesPerWay * way + index] = tag;
      valid[linesPerWay * way + index] = true;
    }
  };
  
  int timeQuantum, levels, 
    ways[8], linesPerWay[8], bytesPerLine[8], accessTime[8];
  int iWays, iLinesPerWay, iBytesPerLine, iAccessTime;
  uint64_t cacheCycles;
  TagArray *tagArrays;
  TagArray *iTagArray;

  unsigned int doAccess(uint64_t p_addr, int startLevel = 0) {
    unsigned int cycles = 0;

    /*Perform a cache access and return required number of cycles.*/
    int i;
    for (i = startLevel; i < levels; i++) {
      /* Break p_addr into tag, index, and offset for this cache level. */
      uint32_t tag = p_addr / bytesPerLine[i] / linesPerWay[i];
      uint32_t index = (p_addr / bytesPerLine[i]) & (linesPerWay[i] - 1);
      if (tagArrays[i].findTag(index, tag)) break; /* Hit => No more cycles */
      
      /* Miss => Incur penalty for miss at this level, place tag in array. */
      cycles += accessTime[i];
      tagArrays[i].setTag(rand()%ways[i], index, tag); /*Random replacement.*/
    }

    return cycles;
  }

public:
  CacheModel(): CallbackHandler(), MemOpHandler(), cacheCycles(0) {
    /* Init parameters based on configuration. */
    int n_ways = 0, n_sets = 0, n_lines = 0, n_fill=0;
    for (
      ParamIt i = (*model_params)->begin(); 
      i != (*model_params)->end(); i++
    ) {
      if (i->first.compare("__cache_interval") == 0) 
        sscanf(i->second.c_str(), "%d", &timeQuantum);
      if (i->first.compare("__cache_iways") == 0)
        sscanf(i->second.c_str(), "%d", &iWays);
      if (i->first.compare("__cache_isets") == 0)
        sscanf(i->second.c_str(), "%d", &iLinesPerWay);
      if (i->first.compare("__cache_iline") == 0)
        sscanf(i->second.c_str(), "%d", &iBytesPerLine);
      if (i->first.compare("__cache_ifill") == 0)
        sscanf(i->second.c_str(), "%d", &iAccessTime);
      if (i->first.compare("__cache_levels") == 0) {
        sscanf(i->second.c_str(), "%d", &levels);
        if (levels > 8) levels = 8;
      }
      if (i->first.compare(0, 12, "__cache_ways") == 0) {
        sscanf(i->second.c_str(), "%d", &ways[n_ways++]);
        if (n_ways == levels) n_ways--;
      }
      if (i->first.compare(0, 12, "__cache_sets") == 0) {
        sscanf(i->second.c_str(), "%d", &linesPerWay[n_sets++]);
        if (n_sets == levels) n_sets--;
      }
      if (i->first.compare(0, 12, "__cache_line") == 0) {
        sscanf(i->second.c_str(), "%d", &bytesPerLine[n_lines++]);
        if (n_lines == levels) n_lines--;
      }
      if (i->first.compare(0, 12, "__cache_fill") == 0) {
        sscanf(i->second.c_str(), "%d", &accessTime[n_fill++]);
        if (n_fill == levels) n_fill--;
      }
    }

    /* Init tag arrays. */
    tagArrays = (TagArray*)malloc(sizeof(TagArray) * levels);
    if (tagArrays == 0) throw(std::bad_alloc());
    for (int i = 0; i < levels; i++) 
      new(tagArrays+i) TagArray(linesPerWay[i], bytesPerLine[i]);

    iTagArray = new TagArray(iLinesPerWay, iBytesPerLine);

    /* Schedule next callback simulation. */
    callbackRequest(this, timeQuantum);
  }

  ~CacheModel() {
    /* Free tag arrays. */
    for (int i = 0; i < levels; i++) tagArrays[i].~TagArray();
    free(tagArrays);
    delete iTagArray;
  }

  /* The callback event handler. */
  virtual void callback(int vm_idx, uint64_t cycle) {
    /* Schedule another callback, set IPC, reset cycle count. */
    double ipc = (double)timeQuantum/(cacheCycles + timeQuantum);
    setIpc(ipc);
    callbackRequest(this, cycle + (long)(timeQuantum/ipc));
    cacheCycles = 0;
  }

  /* The memory operation event handler. */
  virtual void memOp(
    int vm_idx, uint64_t vaddr, uint64_t paddr, uint8_t size, 
    MemAccessType type
  ) {
    uint64_t p_addr;
    for (p_addr = paddr; p_addr < paddr + size; p_addr++) {
      cacheCycles += doAccess(paddr); /*Run access through cache simulator.*/
    }
  }

  /* The instruction operation event handler. Time instruction fetch. */
  virtual void instruction(
    int vm_idx, uint64_t vaddr, uint64_t paddr, uint8_t size, uint8_t *inst
  ) {
    uint64_t p_addr;
    for (p_addr = paddr; p_addr - paddr < size; p_addr++) {
      uint32_t tag = p_addr / iBytesPerLine / iLinesPerWay; 
      uint32_t index = (p_addr / iBytesPerLine) & (iLinesPerWay - 1);
      if ( !iTagArray->findTag(index, tag) /*Miss in icache*/) {
        cacheCycles += iAccessTime + doAccess(paddr, 1);
        iTagArray->setTag(rand()%iWays, index, tag);
      }
    }
  }

};

/* Module constructor and destructor implementations-- put any necessary
   initialization in the _object_ constructor-- just instantiate it here. */
void __init(void) {
  mod_obj = new CacheModel();
}

