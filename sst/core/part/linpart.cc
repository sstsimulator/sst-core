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

#include <sst_config.h>
#include <sst/core/part/linpart.h>

using namespace std;

SSTLinearPartition::SSTLinearPartition(int mpiranks, int verbosity) {
	rankcount = mpiranks;
	partOutput = new Output("LinearPartition ", verbosity, 0, SST::Output::STDOUT);
}

void SSTLinearPartition::performPartition(ConfigGraph* graph) {
	assert(rankcount > 0);

	ConfigComponentMap_t& compMap = graph->getComponentMap();

	// const int componentCount = compMap.size();
	const int componentCount = graph->getNumComponents();
	const int componentRemainder = componentCount % rankcount;
	const int componentPerRank = componentCount / rankcount;

	partOutput->verbose(CALL_INFO, 1, 0, "Performing a linear partition scheme for simulation model.\n");
	partOutput->verbose(CALL_INFO, 1, 0, "Expected linear scheme:\n");
	partOutput->verbose(CALL_INFO, 1, 0, "- Component Count:                  %10d\n", componentCount);
	partOutput->verbose(CALL_INFO, 1, 0, "- Approx. Components per Rank:      %10d\n", componentPerRank);
	partOutput->verbose(CALL_INFO, 1, 0, "- Remainder (non-balanced dist.):   %10d\n", componentRemainder);

	int currentAllocatingRank = 0;
	int componentsOnCurrentRank = 0;

	for(ConfigComponentMap_t::iterator compItr = compMap.begin();
		compItr != compMap.end();
		compItr++) {

		compItr->rank = currentAllocatingRank;
		componentsOnCurrentRank++;

		if(currentAllocatingRank < componentRemainder) {
			if(componentsOnCurrentRank == (componentPerRank + 1)) {
				componentsOnCurrentRank = 0;
				currentAllocatingRank++;
			}
		} else {
			if(componentsOnCurrentRank == componentPerRank) {
				componentsOnCurrentRank = 0;
				currentAllocatingRank++;
			}
		}
	}

	partOutput->verbose(CALL_INFO, 1, 0, "Linear partition scheme completed.\n");
}
