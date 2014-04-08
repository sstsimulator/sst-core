
#ifndef SST_CORE_MODEL_H
#define SST_CORE_MODEL_H

#include <sst/core/configGraph.h>

namespace SST {

/** Base class for Model Generation
 */
class SSTModelDescription {

	public:
		SSTModelDescription();
		virtual ~SSTModelDescription() {};
        /** Create the ConfigGraph
         *
         * This function should be overridden by subclasses.
         *
         * This function is responsible for reading any configuration
         * files and generating a ConfigGraph object.
         */
		virtual ConfigGraph* createConfigGraph() = 0;

};

}

#endif
