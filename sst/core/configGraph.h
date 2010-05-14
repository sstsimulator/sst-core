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


#ifndef SST_CONFIGGRAPH_H
#define SST_CONFIGGRAPH_H

#include "sst/core/graph.h"
#include "sst/core/sdl.h"

namespace SST {

class Simulation;

extern void makeGraph(Simulation *sim, SDL_CompMap_t& map, Graph& graph );
extern int findMinPart(Graph &graph);

} // namespace SST

#endif // SST_CONFIGGRAPH_H
