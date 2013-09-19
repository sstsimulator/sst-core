
#include <sst/core/part/linpart.h>

using namespace std;

SSTLinearPartition::SSTLinearPartition(int mpiranks, int verbosity) {
	rankcount = mpiranks;
	partOutput = new Output("LinearPartition ", verbosity, 0, SST::Output::STDOUT);
}

void SSTLinearPartition::performPartition(ConfigGraph* graph) {
	assert(rankcount > 0);

	ConfigComponentMap_t& compMap = graph->getComponentMap();

	const int componentCount = compMap.size();
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

		partOutput->verbose(CALL_INFO, 1, 0, "> Allocating component: %lu to rank: %d\n",
			compItr->first, currentAllocatingRank);

		compItr->second->rank = currentAllocatingRank;
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
