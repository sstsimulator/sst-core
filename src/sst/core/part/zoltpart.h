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


#ifndef SST_CORE_PART_ZOLT
#define SST_CORE_PART_ZOLT


#ifdef HAVE_ZOLTAN

#include <sst/core/part/sstpart.h>
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

using namespace SST;
using namespace SST::Partition;

namespace SST {
namespace Partition {

/**
	\class SSTZoltanPartition creates a partitioner interface to the
	Zoltan partioner library developed by Sandia National Labs. This is
	an option to partition simulations if the user has configured SST
	to find and compile with the Zoltan external dependency.
*/
class SSTZoltanPartition : public SST::Partition::SSTPartitioner {

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
    
    SST_ELI_REGISTER_PARTITIONER(SSTZoltanPartition,"sst","zoltan","zoltan parallel partitioner")

    SST_ELI_DOCUMENT_VERSION(1,0,0)
};

}
}

#endif // End of HAVE_ZOLTAN

#endif
