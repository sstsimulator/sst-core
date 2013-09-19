
#ifndef SST_CORE_PART_LINEAR
#define SST_CORE_PART_LINEAR

#include <sst/core/part/sstpart.h>
#include <sst/core/output.h>

using namespace SST;
using namespace SST::Partition;

namespace SST {
namespace Partition {

class SSTLinearPartition : public SST::Partition::SSTPartitioner {

	public:
		SSTLinearPartition(int mpiRankCount, int verbosity);
		void performPartition(ConfigGraph* graph);

	protected:
		int rankcount;
		Output* partOutput;

};

}
}

#endif
