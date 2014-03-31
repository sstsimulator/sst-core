
#ifndef SST_CORE_MODEL_H
#define SST_CORE_MODEL_H

#include <sst/core/configGraph.h>

namespace SST {

class SSTModelDescription {

	public:
		SSTModelDescription();
		virtual ~SSTModelDescription() {};
		virtual ConfigGraph* createConfigGraph() = 0;

};

}

#endif
