// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_PART_SELF
#define SST_CORE_PART_SELF

#include <sst/core/part/sstpart.h>

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
		/**
			Creates a new self partition scheme.
		*/
		SSTSelfPartition();

		/**
			Performs a partition of an SST simulation configuration
			\param graph The simulation configuration to partition
		*/
		void performPartition(ConfigGraph* graph);

        bool requiresConfigGraph() { return true; }
        bool spawnOnAllRanks() { return false; }

        static SSTPartitioner* allocate(RankInfo total_ranks, RankInfo my_rank, int verbosity) {
            return new SSTSelfPartition();
    }
        
	private:
        static bool initialized;
};

}
}

#endif
