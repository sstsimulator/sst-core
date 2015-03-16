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


#ifndef SST_CORE_PART_ZOLT
#define SST_CORE_PART_ZOLT


#ifdef HAVE_ZOLTAN

#include <sst/core/part/sstpart.h>
#include <sst/core/output.h>

#include <zoltan.h>
#include <mpi.h>

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

	public:
		/**
			Create a Zoltan-based partition scheme
			\param verbosity Verbosity level with which messages and information are generated
		*/
		SSTZoltanPartition(int verbosity);
		~SSTZoltanPartition();

		/**
			Performs a partition of an SST partition graph. Components in the graph
			have their setRank() attribute set based on the partition scheme computed
			by Zoltan.
			\param graph An SST partition graph
		*/
		void performPartition(PartitionGraph* graph);

        bool requiresConfigGraph() { return false; }

        bool spawnOnAllRanks() { return true; }
        
        static SSTPartitioner* allocate(int total_ranks, int my_rank, int verbosity) {
            return new SSTZoltanPartition(verbosity);
        }
        
	protected:
		void initZoltan();
		int rankcount;
		struct Zoltan_Struct * zolt_config;
		int rank;

        static bool initialized;
};

}
}

#endif // End of HAVE_ZOLTAN

#endif
