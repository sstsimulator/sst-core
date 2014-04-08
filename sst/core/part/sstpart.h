
#ifndef SST_CORE_PART_BASE
#define SST_CORE_PART_BASE

#include <sst/core/configGraph.h>

namespace SST {
namespace Partition {

/**
 * Base class for Partitioning graphs
 */
class SSTPartitioner
{
	public:
		SSTPartitioner();
		virtual ~SSTPartitioner() {};
        /** Function to be overriden by subclasses
         *
         * Performs the partitioning of the Graph
         *
         * Result of this function is that every ConfigComponent in
         * graph has a Rank applied to it.
         */
		virtual void performPartition(ConfigGraph* graph) = 0;
};

}
}

#endif
