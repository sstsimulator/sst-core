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

#ifndef __EVENTQUEUE_H
/*----------------------------------------------------------+----------------\
| eventqueue.h                                              | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| Event Queue for managing callbacks in Psst.                                |
\---------------------------------------------------------------------------*/
#define __EVENTQUEUE_H

#include "models/model.h"
#include <map>
#include <utility>

class EventQueue {
public:
  uint64_t current_cycle;
  struct EventType {
    EventType(Models::CallbackHandler* model): m(model) {}
    Models::CallbackHandler *m;
  };
  EventQueue(): current_cycle(0), eq() {}
  void add(Models::CallbackHandler* m, uint64_t cycle); // Add an event.
  uint64_t cycles();  // Get cycles until next event.
  uint64_t advance(); // Advance to next event, return number of cycles
                      // passed.
private:
  template <class T> struct Compare { 
    bool operator()(const T& l, const T& r) { return l < r; }
  };
  std::multimap<uint64_t, EventType, Compare<uint64_t> > eq;
};

#endif

