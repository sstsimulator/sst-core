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

class SSTZoltanPartition : public SST::Partition::SSTPartitioner {

	public:
		SSTZoltanPartition(int verbosity);
		~SSTZoltanPartition();
		void performPartition(ConfigGraph* graph);

	protected:
		void initZoltan();
		int rankcount;
		Output* partOutput;
		struct Zoltan_Struct * zolt_config;
		int rank;

};

}
}

#endif // End of HAVE_ZOLTAN

#endif
