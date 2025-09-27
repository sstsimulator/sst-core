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

#ifndef SST_CORE_CORETEST_OVERHEADMEASURE_H
#define SST_CORE_CORETEST_OVERHEADMEASURE_H

#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/rng/marsaglia.h"

#include <vector>

namespace SST::CoreTestOverhead {

class OverheadMeasure : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        OverheadMeasure,
        "coreTestElement",
        "overhead_measure",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Element to measure overheads in the ConfigGraph and BaseComponent base class",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "id", "ID of component", "" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS()

    SST_ELI_DOCUMENT_PORTS(
        {"left_%d", "dth left port ",  { "NullEvent", "" } },
        {"right_%d", "dth right port ",  { "NullEvent", "" } }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    OverheadMeasure(SST::ComponentId_t id, SST::Params& params);
    OverheadMeasure()  = default;
    ~OverheadMeasure() = default;

    void init(unsigned int UNUSED(phase)) override {}
    void setup() override {}
    void complete(unsigned int UNUSED(phase)) override {}
    void finish() override {}

private:
    int id_;
    int ports_;

    void handleEvent(SST::Event* ev, int port);
    bool clockTic(Cycle_t cycle);

    std::vector<Link*> links_;
};

} // namespace SST::CoreTestOverhead

#endif // SST_CORE_CORETEST_OVERHEADMEASURE_H
