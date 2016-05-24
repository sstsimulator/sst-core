// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_PART_BASE
#define SST_CORE_PART_BASE

#include <sst/core/configGraph.h>
#include <sst/core/rankInfo.h>

#include <map>

namespace SST {
namespace Partition {

/**
 * Base class for Partitioning graphs
 */
class SSTPartitioner
{

public:

    typedef SSTPartitioner* (*partitionerAlloc)(RankInfo total_ranks, RankInfo my_rank, int verbosity);
private:
    static std::map<std::string, SSTPartitioner::partitionerAlloc>& partitioner_allocs();
    static std::map<std::string, std::string>& partitioner_descriptions();
    
public:

    SSTPartitioner();
    virtual ~SSTPartitioner() {};
    
    static bool addPartitioner(const std::string name, const SSTPartitioner::partitionerAlloc alloc, const std::string description);
    static SSTPartitioner* getPartitioner(std::string name, RankInfo total_ranks, RankInfo my_rank, int verbosity);

    static const std::map<std::string, std::string>& getDescriptionMap() { return partitioner_descriptions(); }

    
    /** Function to be overriden by subclasses
     *
     * Performs the partitioning of the Graph using the PartitionGraph object.
     *
     * Result of this function is that every ConfigComponent in
     * graph has a Rank applied to it.
     */
    virtual void performPartition(PartitionGraph* graph) {}

    /** Function to be overriden by subclasses
     *
     * Performs the partitioning of the Graph using the ConfigGraph
     * object.  The consequence of using ConfigGraphs is that no-cut
     * links are not supported.
     *
     * Result of this function is that every ConfigComponent in
     * graph has a Rank applied to it.
     */
    virtual void performPartition(ConfigGraph* graph) {}
    
    virtual bool requiresConfigGraph() { return false; }
    
    virtual bool spawnOnAllRanks() { return false; }
    // virtual bool supportsPartialPartitionInput() { return false; }


};

}
}

#endif
