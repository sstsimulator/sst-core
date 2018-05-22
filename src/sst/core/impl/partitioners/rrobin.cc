// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/impl/partitioners/rrobin.h"

#include <sst/core/warnmacros.h>

#include <string>

#include "sst/core/configGraph.h"

using namespace std;

namespace SST {
namespace IMPL {
namespace Partition {

SSTRoundRobinPartition::SSTRoundRobinPartition(RankInfo world_size, RankInfo UNUSED(my_rank), int UNUSED(verbosity)) :
    SSTPartitioner(),
    world_size(world_size)
{
}

void SSTRoundRobinPartition::performPartition(PartitionGraph* graph) {
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
} // namespace IMPL
} // namespace SST
