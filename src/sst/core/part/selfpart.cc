// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/part/selfpart.h>

using namespace std;

bool SSTSelfPartition::initialized = SSTPartitioner::addPartitioner("self",&SSTSelfPartition::allocate, "Used when partitioning is already specified in the configuration file.");

SSTSelfPartition::SSTSelfPartition() {}

void SSTSelfPartition::performPartition(ConfigGraph* graph) {
    return;
}
