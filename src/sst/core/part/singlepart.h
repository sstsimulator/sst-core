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


#ifndef SST_CORE_PART_SINGLE
#define SST_CORE_PART_SINGLE

#include <sst/core/part/sstpart.h>
#include <sst/core/elementinfo.h>

using namespace SST;
using namespace SST::Partition;

namespace SST {
namespace Partition {


    
/**
   Single partitioner is a virtual partitioner used for serial jobs.
   It simply ensures that all components are assigned to rank 0.
*/
class SSTSinglePartition : public SST::Partition::SSTPartitioner {

public:
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
    
    
    SST_ELI_REGISTER_PARTITIONER(SSTSinglePartition,"sst","single","Allocates all components to rank 0.  Automatically selected for serial jobs.")
};

}
}

#endif
