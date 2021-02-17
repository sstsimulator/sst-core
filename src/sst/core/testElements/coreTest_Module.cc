// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>

#include "sst/core/testElements/coreTest_Module.h"

using namespace SST::CoreTestModule;

CoreTestModuleExample::CoreTestModuleExample(SST::Params& params) {
	modName = params.find<std::string>("modulename", "");
}

void CoreTestModuleExample::printName() {
	printf("Name: %s\n", modName.c_str());
}
