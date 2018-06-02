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


#ifndef SST_CORE_PART_BASE
#define SST_CORE_PART_BASE

#include <sst/core/rankInfo.h>
#include <sst/core/warnmacros.h>

#include <map>

namespace SST {

class ConfigGraph;
class PartitionGraph;

namespace Partition {

/**
 * Base class for Partitioning graphs
 */
class SSTPartitioner
{

public:

    SSTPartitioner() {}
    virtual ~SSTPartitioner() {}
    
    /** Function to be overridden by subclasses
     *
     * Performs the partitioning of the Graph using the PartitionGraph object.
     *
     * Result of this function is that every ConfigComponent in
     * graph has a Rank applied to it.
     */
    virtual void performPartition(PartitionGraph* UNUSED(graph)) {}

    /** Function to be overridden by subclasses
     *
     * Performs the partitioning of the Graph using the ConfigGraph
     * object.  The consequence of using ConfigGraphs is that no-cut
     * links are not supported.
     *
     * Result of this function is that every ConfigComponent in
     * graph has a Rank applied to it.
     */
    virtual void performPartition(ConfigGraph* UNUSED(graph)) {}
    
    virtual bool requiresConfigGraph() { return false; }
    
    virtual bool spawnOnAllRanks() { return false; }
    // virtual bool supportsPartialPartitionInput() { return false; }


};

}
}

#endif
