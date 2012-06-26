
#include <sst_config.h>

#include <string>
#include <vector>
#include <map>
#include <set>

#include <boost/mpi.hpp>

#include "sst/core/configGraph.h"
#include "sst/core/graph.h"
#include "sst/core/sdl.h"
#include "sst/core/sst_types.h"
#include "sst/core/serialization/core.h"

using namespace std;

namespace SST {

	void rrobin_partition(ConfigGraph* graph, int world_size) {
		ConfigComponentMap_t compMap = graph->getComponentMap();
		int counter = 0;
		
		for(ConfigComponentMap_t::const_iterator compItr = compMap.begin();
			compItr != compMap.end();
			compItr++) {
			
			(*compItr).second->rank = (counter % world_size);
			counter++;
		}
	}

}
