// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/part/singlepart.h>

using namespace std;

bool SSTSinglePartition::initialized = SSTPartitioner::addPartitioner("single",&SSTSinglePartition::allocate, "Allocates all components to rank 0.  Automatically selected for serial jobs.");

SSTSinglePartition::SSTSinglePartition() {}

void SSTSinglePartition::performPartition(ConfigGraph* graph) {
    graph->setComponentRanks(0);
}
