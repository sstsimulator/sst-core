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

#ifndef SST_CORE_CORETEST_UTILITY_H
#define SST_CORE_CORETEST_UTILITY_H

#include "sst/core/component.h"

namespace SST::CoreTestUtility {

class coreTestUtility : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestUtility,
        "coreTestElement",
        "coreTestUtility",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Runs self-checking tests of core utility classes during construction",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "test", "Name of the utility test to run (e.g. sparsevectormap_lookup)", NULL}
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

    coreTestUtility(SST::ComponentId_t id, SST::Params& params);
    ~coreTestUtility() {}
};

} // namespace SST::CoreTestUtility

#endif // SST_CORE_CORETEST_UTILITY_H
