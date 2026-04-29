// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include "sst_config.h"

#include "sst/core/configGraphOutput.h"

#include "sst/core/simulation.h"
#include "sst/core/util/filesystem.h"

namespace SST::Core {

ConfigGraphOutput::ConfigGraphOutput(const char* path)
{
    SST::Util::Filesystem filesystem = Simulation::filesystem;
    outputFile                       = filesystem.fopen(path, "wt");
}

} // namespace SST::Core
