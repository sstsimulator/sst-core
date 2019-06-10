// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_IMPL_PARTITONERS_PARMETISPART_H
#define SST_CORE_IMPL_PARTITONERS_PARMETISPART_H

#ifdef HAVE_PARMETIS

#include <sst/core/sstpart.h>
#include <sst/core/output.h>
#include <sst/core/eli/elementinfo.h>

#include <mpi.h>
#include <parmetis.h>

namespace SST {
namespace IMPL {
namespace Partition {

/**
	\class SSTParMETISPartition creates a partitioner interface to the
	ParMETIS partitioner library. This is
	an option to partition simulations if the user has configured SST
	to find and compile with the ParMETIS external dependency.
*/
class SSTParMETISPartition : public SST::Partition::SSTPartitioner {

public:

    SST_ELI_REGISTER_PARTITIONER(
        SSTParMETISPartition,
        "sst",
        "parmetis",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "ParMETIS Parallel Partitioner")

protected:
    RankInfo rank;
    
public:
    /**
       Create a ParMETIS-based partition scheme
       \param verbosity Verbosity level with which messages and information are generated
    */
    SSTParMETISPartition(RankInfo world_size, RankInfo my_rank, int verbosity);
    ~SSTParMETISPartition();
    
    /**
       Performs a partition of an SST partition graph. Components in the graph
       have their setRank() attribute set based on the partition scheme computed
       by ParMETIS.

       \param graph An SST partition graph
    */
    void performPartition(PartitionGraph* graph) override;

    void performPartition(ConfigGraph* graph) override {
	SST::Partition::SSTPartitioner::performPartition(graph);
    }
    
    bool requiresConfigGraph() override { return false; }    
    bool spawnOnAllRanks() override { return true; }
    
};

}
}
}
#endif // End of HAVE_PARMETIS

#endif
