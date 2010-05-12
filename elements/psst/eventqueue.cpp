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

/*----------------------------------------------------------+----------------\
| eventqueue.cpp                                            | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| Event Queue for managing callbacks in Psst.                                |
\---------------------------------------------------------------------------*/
#include "models/model.h"
#include <map>
#include <utility>

#include "eventqueue.h"

using Models::CallbackHandler;
using std::pair; using std::multimap;

void EventQueue::add(CallbackHandler *m, uint64_t cycle) {
  eq.insert(eq.end(), pair<uint64_t, EventQueue::EventType>(
    cycle, EventQueue::EventType(m)
  ));
}

uint64_t EventQueue::cycles() {
  if (eq.empty()) return 0;
  return eq.begin()->first - current_cycle;
}

uint64_t EventQueue::advance() {
  if (eq.empty()) return 0;
  uint64_t next_cycle = eq.begin()->first;
  uint64_t elapsed = next_cycle - current_cycle;
  current_cycle = next_cycle;

  CallbackHandler *m = eq.begin()->second.m;
  if (m) m->callback(0, current_cycle);

  eq.erase(eq.begin());

  return elapsed;
}
