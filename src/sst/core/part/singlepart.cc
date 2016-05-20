// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/configGraph.h>
#include <sst/core/part/singlepart.h>

using namespace std;

bool SSTSinglePartition::initialized = SSTPartitioner::addPartitioner("single",&SSTSinglePartition::allocate, "Allocates all components to rank 0.  Automatically selected for serial jobs.");

using namespace SST::Partition;

SSTSinglePartition::SSTSinglePartition() {}

void SSTSinglePartition::performPartition(PartitionGraph* graph) {

	PartitionComponentMap_t& compMap = graph->getComponentMap();

	for(auto compItr = compMap.begin(); compItr != compMap.end(); compItr++) {
		compItr->rank = RankInfo(0,0);
	}

}
