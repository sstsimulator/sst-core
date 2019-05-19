#include <sst/core/sstpart.h>

namespace SST {
namespace Partition {

SST_ELI_DEFINE_INFO_EXTERN(SSTPartitioner)
SST_ELI_DEFINE_CTOR_EXTERN(SSTPartitioner)

void
SSTPartitioner::performPartition(PartitionGraph* UNUSED(graph))
{
  Output &output = Output::getDefaultObject();
  output.fatal(CALL_INFO, 1, "ERROR: chosen partitioner does not support PartitionGraph");
}

void
SSTPartitioner::performPartition(ConfigGraph* UNUSED(graph))
{
  Output &output = Output::getDefaultObject();
  output.fatal(CALL_INFO, 1, "ERROR: chosen partitioner does not support ConfigGraph");
}

}
}

