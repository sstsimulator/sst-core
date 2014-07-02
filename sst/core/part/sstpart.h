// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_PART_BASE
#define SST_CORE_PART_BASE

#include <sst/core/configGraph.h>

namespace SST {
namespace Partition {

/**
 * Base class for Partitioning graphs
 */
class SSTPartitioner
{
	public:
		SSTPartitioner();
		virtual ~SSTPartitioner() {};
        /** Function to be overriden by subclasses
         *
         * Performs the partitioning of the Graph
         *
         * Result of this function is that every ConfigComponent in
         * graph has a Rank applied to it.
         */
		virtual void performPartition(PartitionGraph* graph) = 0;
};

}
}

#endif
