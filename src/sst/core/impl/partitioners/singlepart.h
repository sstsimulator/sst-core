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


#ifndef SST_CORE_IMPL_PARTITONERS_SINGLEPART_H
#define SST_CORE_IMPL_PARTITONERS_SINGLEPART_H

#include <sst/core/sstpart.h>
#include <sst/core/elementinfo.h>

namespace SST {
namespace IMPL {
namespace Partition {


    
/**
   Single partitioner is a virtual partitioner used for serial jobs.
   It simply ensures that all components are assigned to rank 0.
*/
class SSTSinglePartition : public SST::Partition::SSTPartitioner {

public:
    SST_ELI_REGISTER_PARTITIONER(
        SSTSinglePartition,
        "sst",
        "single",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Allocates all components to rank 0.  Automatically selected for serial jobs.")

    /**
       Creates a new single partition scheme.
    */
    SSTSinglePartition(RankInfo total_ranks, RankInfo my_rank, int verbosity);
    
    /**
       Performs a partition of an SST simulation configuration
       \param graph The simulation configuration to partition
    */
    void performPartition(PartitionGraph* graph) override;
    
    bool requiresConfigGraph() override { return false; }
    bool spawnOnAllRanks() override { return false; }
    
    
};

}
}
}

#endif
