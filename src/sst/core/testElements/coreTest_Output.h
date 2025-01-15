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

#ifndef SST_CORE_CORETEST_OUTPUT_H
#define SST_CORE_CORETEST_OUTPUT_H

#include "sst/core/component.h"

namespace SST {
namespace CoreTestSerialization {

class coreTestOutput : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestOutput,
        "coreTestElement",
        "coreTestOutput",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Test element for output objects",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "test", "Type of output test to perform", NULL}
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestOutput(SST::ComponentId_t id, SST::Params& params);
    ~coreTestOutput() {}

private:
};

} // namespace CoreTestSerialization
} // namespace SST

#endif // SST_CORE_CORETEST_OUTPUT_H
