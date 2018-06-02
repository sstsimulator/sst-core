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


#ifndef SST_CORE_IMPL_PARTITONERS_ZOLTPART_H
#define SST_CORE_IMPL_PARTITONERS_ZOLTPART_H


#ifdef HAVE_ZOLTAN

#include <sst/core/sstpart.h>
#include <sst/core/output.h>
#include <sst/core/elementinfo.h>

// SST and ZOLTANÕs configurations are conflicting on the SST_CONFIG_HAVE_MPI definition.  
// So temporarily shut down SSTÕs SST_CONFIG_HAVE_MPI, then allow ZOLTANÕs SST_CONFIG_HAVE_MPI to 
//be defined, then reset SSTÕs SST_CONFIG_HAVE_MPI.
#ifdef SST_CONFIG_HAVE_MPI 
#undef SST_CONFIG_HAVE_MPI  
#include <zoltan.h>
#include <mpi.h>
#define SST_CONFIG_HAVE_MPI
#endif

namespace SST {
namespace IMPL {
namespace Partition {

/**
	\class SSTZoltanPartition creates a partitioner interface to the
	Zoltan partitioner library developed by Sandia National Labs. This is
	an option to partition simulations if the user has configured SST
	to find and compile with the Zoltan external dependency.
*/
class SSTZoltanPartition : public SST::Partition::SSTPartitioner {

public:

    SST_ELI_REGISTER_PARTITIONER(
        SSTZoltanPartition,
        "sst",
        "zoltan",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "zoltan parallel partitioner")

protected:
    void initZoltan();
    RankInfo rankcount;
    struct Zoltan_Struct * zolt_config;
    RankInfo rank;
    
public:
    /**
       Create a Zoltan-based partition scheme
       \param verbosity Verbosity level with which messages and information are generated
    */
    SSTZoltanPartition(RankInfo world_size, RankInfo my_rank, int verbosity);
    ~SSTZoltanPartition();
    
    /**
       Performs a partition of an SST partition graph. Components in the graph
       have their setRank() attribute set based on the partition scheme computed
       by Zoltan.
       \param graph An SST partition graph
    */
    void performPartition(PartitionGraph* graph) override;
    
    bool requiresConfigGraph() override { return false; }
    
    bool spawnOnAllRanks() override { return true; }
    
};

}
}
}
#endif // End of HAVE_ZOLTAN

#endif
