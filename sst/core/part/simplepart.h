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
#ifndef SST_CORE_PART_SIMPLEPART_H
#define SST_CORE_PART_SIMPLEPART_H
namespace SST {
  class PartitionGraph;
	/**
		Implements the "simple" partitioner which uses latency
		information in the SST component configuration to perform
		approximate load balancing of components over MPI ranks. Note
		that this scheme does run if SST is run in serial.
	*/
	void simple_partition(PartitionGraph* graph, int world_size);

} //namespace SST
#endif //SST_CORE_PART_SIMPLERPART_H
