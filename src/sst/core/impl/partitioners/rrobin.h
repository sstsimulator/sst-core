// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#ifndef SST_CORE_IMPL_PARTITONERS_RROBIN_H
#define SST_CORE_IMPL_PARTITONERS_RROBIN_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/sstpart.h"

namespace SST::IMPL::Partition {

class SSTRoundRobinPartition : public SST::Partition::SSTPartitioner
{

public:
    SST_ELI_REGISTER_PARTITIONER(
        SSTRoundRobinPartition,
        "sst",
        "roundrobin",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Partitions components using a simple round robin scheme based on ComponentID.  "
        "Sequential IDs will be placed on different ranks.")

private:
    RankInfo world_size;

public:
    SSTRoundRobinPartition(RankInfo world_size, RankInfo my_rank, int verbosity);

    /**
       Performs a partition of an SST simulation configuration
       \param graph The simulation configuration to partition
    */
    void performPartition(PartitionGraph* graph) override;

    bool requiresConfigGraph() override { return false; }
    bool spawnOnAllRanks() override { return false; }
};

} // namespace SST::IMPL::Partition

#endif // SST_CORE_IMPL_PARTITONERS_RROBIN_H
