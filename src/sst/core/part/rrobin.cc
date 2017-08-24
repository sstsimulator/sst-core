// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/part/rrobin.h"
#include <sst/core/warnmacros.h>

#include <string>

#include "sst/core/configGraph.h"

using namespace std;

namespace SST {
namespace Partition {

SSTRoundRobinPartition::SSTRoundRobinPartition(RankInfo world_size, RankInfo UNUSED(my_rank), int UNUSED(verbosity)) :
    SSTPartitioner(),
    world_size(world_size)
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
