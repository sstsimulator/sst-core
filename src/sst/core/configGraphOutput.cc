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
//

#include "sst_config.h"

#include "sst/core/configGraphOutput.h"

#include "sst/core/simulation_impl.h"
#include "sst/core/util/filesystem.h"

namespace SST {
namespace Core {

ConfigGraphOutput::ConfigGraphOutput(const char* path)
{
    SST::Util::Filesystem filesystem = Simulation_impl::filesystem;
    outputFile                       = filesystem.fopen(path, "wt");
}


} // namespace Core
} // namespace SST
