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

#ifndef SST_CORE_CORETEST_MODULE_H
#define SST_CORE_CORETEST_MODULE_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/module.h>
#include <vector>

namespace SST {
namespace CoreTestModule {

class CoreTestModuleExample : public SST::Module
{

public:
    CoreTestModuleExample(SST::Params& params);

    SST_ELI_REGISTER_MODULE(
        CoreTestModuleExample,
        "coreTestElement",
        "CoreTestModule",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "CoreTest module to demonstrate interface.",
        "SST::CoreTestModule::CoreTestModuleInterface")

    SST_ELI_DOCUMENT_PARAMS(
        {"modulename", "Name to give this module", ""},
    )

    void printName();

private:
    std::string modName;
};

} // namespace CoreTestModule
} // namespace SST

#endif // SST_CORE_CORETEST_MODULE_H
