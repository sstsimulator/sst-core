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


#ifndef SST_CORE_PART_SELF
#define SST_CORE_PART_SELF

#include <sst/core/part/sstpart.h>
#include <sst/core/elementinfo.h>

using namespace SST;
using namespace SST::Partition;

namespace SST {
namespace Partition {


    
/**
   Self partitioner actually does nothing.  It is simply a pass
   through for graphs which have been partitioned during graph
   creation.
*/
class SSTSelfPartition : public SST::Partition::SSTPartitioner {

public:
    SST_ELI_REGISTER_PARTITIONER(
        SSTSelfPartition,
        "sst",
        "self",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Used when partitioning is already specified in the configuration file.")

    /**
       Creates a new self partition scheme.
    */
    SSTSelfPartition(RankInfo UNUSED(total_ranks), RankInfo UNUSED(my_rank), int UNUSED(verbosity)) {}
    
    /**
       Performs a partition of an SST simulation configuration
       \param graph The simulation configuration to partition
    */
    void performPartition(ConfigGraph* UNUSED(graph)) { return; }
    
    bool requiresConfigGraph() { return true; }
    bool spawnOnAllRanks() { return false; }
    

};

}
}

#endif
