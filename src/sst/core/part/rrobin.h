// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#ifndef SST_CORE_PART_RROBIN_H
#define SST_CORE_PART_RROBIN_H

#include <sst/core/part/sstpart.h>

namespace SST {
namespace Partition {

class SSTRoundRobinPartition : public SST::Partition::SSTPartitioner {

public:
    SSTRoundRobinPartition(RankInfo world_size);
    
    /**
       Performs a partition of an SST simulation configuration
       \param graph The simulation configuration to partition
    */
	void performPartition(PartitionGraph* graph);

    bool requiresConfigGraph() { return false; }
    bool spawnOnAllRanks() { return false; }
    
    static SSTPartitioner* allocate(RankInfo total_ranks, RankInfo my_rank, int verbosity) {
        return new SSTRoundRobinPartition(total_ranks);
    }        
        
private:
    RankInfo world_size;
    static bool initialized;
};

} // namespace Partition
} //namespace SST
#endif //SST_CORE_PART_RROBIN_H
