// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

namespace SST {
  class ConfigGraph;
	/**
		Implements the "simple" partitioner which uses latency
		information in the SST component configuration to perform
		approximate load balancing of components over MPI ranks. Note
		that this scheme does run if SST is run in serial.
	*/
	void simple_partition(ConfigGraph* graph, int world_size);

}
