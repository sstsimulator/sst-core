// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_SERIALIZATION_H
#define SST_CORE_CORETEST_SERIALIZATION_H

#include "sst/core/component.h"
#include "sst/core/rng/rng.h"

namespace SST {
namespace CoreTestSerialization {

class coreTestSerialization : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestSerialization,
        "coreTestElement",
        "coreTestSerialization",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Serialization Check Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
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

    coreTestSerialization(SST::ComponentId_t id, SST::Params& params);
    ~coreTestSerialization() {}

private:
    SST::RNG::Random* rng;
};

} // namespace CoreTestSerialization
} // namespace SST

#endif // SST_CORE_CORETEST_SERIALIZATION_H
