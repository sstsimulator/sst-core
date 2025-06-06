// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/sstpart.h"

#include "sst/core/output.h"

namespace SST::Partition {

SST_ELI_DEFINE_INFO_EXTERN(SSTPartitioner)
SST_ELI_DEFINE_CTOR_EXTERN(SSTPartitioner)

DISABLE_WARN_MISSING_NORETURN
void
SSTPartitioner::performPartition(PartitionGraph* UNUSED(graph))
{
    Output& output = Output::getDefaultObject();
    output.fatal(CALL_INFO, 1, "ERROR: chosen partitioner does not support PartitionGraph");
}

void
SSTPartitioner::performPartition(ConfigGraph* UNUSED(graph))
{
    Output& output = Output::getDefaultObject();
    output.fatal(CALL_INFO, 1, "ERROR: chosen partitioner does not support ConfigGraph");
}
REENABLE_WARNING

} // namespace SST::Partition
