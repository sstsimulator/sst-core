
#ifndef SST_CORE_PART_BASE
#define SST_CORE_PART_BASE

#include <sst/core/configGraph.h>

namespace SST {
namespace Partition {

class SSTPartitioner
{
	public:
		SSTPartitioner();
		virtual void performPartition(ConfigGraph* graph) = 0;
};

}
}

#endif
