// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/configGraph.h>
#include <sst/core/part/singlepart.h>
#include <sst/core/warnmacros.h>

using namespace std;

using namespace SST::Partition;

SSTSinglePartition::SSTSinglePartition(RankInfo UNUSED(total_ranks), RankInfo UNUSED(my_rank), int UNUSED(verbosity)) {}

void SSTSinglePartition::performPartition(PartitionGraph* graph) {

	PartitionComponentMap_t& compMap = graph->getComponentMap();

	for(auto compItr = compMap.begin(); compItr != compMap.end(); compItr++) {
		compItr->rank = RankInfo(0,0);
	}

}
