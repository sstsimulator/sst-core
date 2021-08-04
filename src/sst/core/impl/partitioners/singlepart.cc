// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/impl/partitioners/singlepart.h"

#include "sst/core/configGraph.h"
#include "sst/core/warnmacros.h"

using namespace std;

using namespace SST::IMPL::Partition;

SSTSinglePartition::SSTSinglePartition(RankInfo UNUSED(total_ranks), RankInfo UNUSED(my_rank), int UNUSED(verbosity)) {}

void
SSTSinglePartition::performPartition(ConfigGraph* graph)
{
    ConfigComponentMap_t& compMap = graph->getComponentMap();

    for ( auto comp : compMap ) {
        comp->rank = RankInfo(0, 0);
    }
}
