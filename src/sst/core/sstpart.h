// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SSTPART_H
#define SST_CORE_SSTPART_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/rankInfo.h"
#include "sst/core/warnmacros.h"

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
    SST_ELI_DECLARE_BASE(SSTPartitioner)
    SST_ELI_DECLARE_DEFAULT_INFO_EXTERN()
    SST_ELI_DECLARE_CTOR_EXTERN(RankInfo,RankInfo,int)

    SSTPartitioner() {}
    virtual ~SSTPartitioner() {}

    /** Function to be overridden by subclasses
     *
     * Performs the partitioning of the Graph using the PartitionGraph object.
     *
     * Result of this function is that every ConfigComponent in
     * graph has a Rank applied to it.
     */
    virtual void performPartition(PartitionGraph* graph);

    /** Function to be overridden by subclasses
     *
     * Performs the partitioning of the Graph using the ConfigGraph
     * object.  The consequence of using ConfigGraphs is that no-cut
     * links are not supported.
     *
     * Result of this function is that every ConfigComponent in
     * graph has a Rank applied to it.
     */
    virtual void performPartition(ConfigGraph* graph);

    virtual bool requiresConfigGraph() { return false; }

    virtual bool spawnOnAllRanks() { return false; }
    // virtual bool supportsPartialPartitionInput() { return false; }
};

} // namespace Partition
} // namespace SST

#ifndef SST_ELI_REGISTER_PARTITIONER
#define SST_ELI_REGISTER_PARTITIONER(cls, lib, name, version, desc) \
    SST_ELI_REGISTER_DERIVED(SST::Partition::SSTPartitioner,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)
#endif

#endif // SST_CORE_SSTPART_H
