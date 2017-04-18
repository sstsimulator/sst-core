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
#ifndef SST_CORE_PART_SIMPLEPART_H
#define SST_CORE_PART_SIMPLEPART_H

#include <map>
#include <sst/core/sst_types.h>
#include <sst/core/part/sstpart.h>
#include <sst/core/elementinfo.h>
#include <sst/core/configGraph.h>

namespace SST {
namespace Partition{

class SimplePartitioner : public SST::Partition::SSTPartitioner {

private:
    RankInfo world_size;
    uint32_t total_parts;

    RankInfo convertPartNum(uint32_t partNum) {
        return RankInfo(partNum / world_size.thread, partNum % world_size.thread);
    }

	void simple_partition_step(PartitionComponentMap_t& component_map,
			ComponentId_t* setA, const int lengthA, int rankA,
			ComponentId_t* setB, const int lengthB, int rankB,
			std::map<ComponentId_t, std::map<ComponentId_t, SimTime_t>*> timeTable,
			int step);
public:

    SimplePartitioner(RankInfo total_ranks, RankInfo my_rank, int verbosity);
    SimplePartitioner();
    ~SimplePartitioner() {}

    void performPartition(PartitionGraph* graph) override;

    bool requiresConfigGraph() override { return false; }
    bool spawnOnAllRanks() override { return false; }

    SST_ELI_REGISTER_PARTITIONER(SimplePartitioner,"sst","simple","Simple partitioning scheme which attempts to partition on high latency links while balancing number of components per rank.")
    
};

} // namespace partition
} //namespace SST
#endif //SST_CORE_PART_SIMPLERPART_H
