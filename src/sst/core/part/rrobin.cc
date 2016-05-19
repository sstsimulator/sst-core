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
#include "sst/core/serialization.h"
#include "sst/core/part/rrobin.h"

#include <string>

#include "sst/core/configGraph.h"

using namespace std;

namespace SST {
namespace Partition {

bool SSTRoundRobinPartition::initialized = SSTPartitioner::addPartitioner("roundrobin",&SSTRoundRobinPartition::allocate, "Partitions components using a simple round robin scheme based on ComponentID.  Sequential IDs will be placed on different ranks.");

SSTRoundRobinPartition::SSTRoundRobinPartition(RankInfo mpiranks) :
    SSTPartitioner(),
    world_size(mpiranks)
{
}

void SSTRoundRobinPartition::performPartition(PartitionGraph* graph) {
    std::cout << "Round robin partitioning" << std::endl;
    PartitionComponentMap_t& compMap = graph->getComponentMap();
    RankInfo rank(0, 0);

    for(PartitionComponentMap_t::iterator compItr = compMap.begin();
            compItr != compMap.end();
            compItr++) {

        compItr->rank = rank;

        rank.rank++;
        if ( rank.rank == world_size.rank ) {
            rank.thread = (rank.thread+1)%world_size.thread;
            rank.rank = 0;
        }
    }
}

} // namespace Partition
} // namespace SST
