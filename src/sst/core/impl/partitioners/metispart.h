#ifndef SST_CORE_IMPL_PARTITIONERS_METISPART_H
#define SST_CORE_IMPL_PARTITIONERS_METISPART_H

#include "sst/core/sst_config.h"

#ifdef HAVE_METIS

#include "sst/core/sstpart.h"
#include "sst/core/output.h"
#include "sst/core/eli/elementinfo.h"


namespace SST {
namespace IMPL {
namespace Partition {

class SSTMetisPartition : public SST::Partition::SSTPartitioner {

public:

    SST_ELI_REGISTER_PARTITIONER(
        SSTMetisPartition,
        "sst",
        "metis",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "metis graph partitioner");

    SSTMetisPartition(RankInfo world_size, RankInfo my_rank, int verbosity);
    ~SSTMetisPartition() {
        if (partOutput) delete partOutput;
    }

    void performPartition(PartitionGraph* graph) override;

    bool requiresConfigGraph() override { return false; }
    bool spawnOnAllRanks() override { return false; }

private:
    /** Number of ranks in the simulation */
    RankInfo rankcount;
    Output* partOutput=nullptr;
};
}
}
}

#endif //End of HAVE_METIS


#endif
