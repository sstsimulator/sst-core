
#ifndef SST_CORE_PART_ZOLT
#define SST_CORE_PART_ZOLT

#include <sst_config.h>

#ifdef HAVE_ZOLTAN

#include <sst/core/part/sstpart.h>
#include <sst/core/output.h>

#include <zoltan.h>
#include <mpi.h>

using namespace SST;
using namespace SST::Partition;

namespace SST {
namespace Partition {

class SSTZoltanPartition : public SST::Partition::SSTPartitioner {

	public:
		SSTZoltanPartition(int verbosity);
		~SSTZoltanPartition();
		void performPartition(ConfigGraph* graph);

	protected:
		void initZoltan();
		int rankcount;
		Output* partOutput;
		struct Zoltan_Struct * zolt_config;
		int rank;

};

}
}

#endif // End of HAVE_ZOLTAN

#endif
