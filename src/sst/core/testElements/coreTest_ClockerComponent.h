// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_CLOCKERCOMPONENT_H
#define SST_CORE_CORETEST_CLOCKERCOMPONENT_H

#include "sst/core/component.h"

namespace SST {
namespace CoreTestClockerComponent {

class coreTestClockerComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestClockerComponent,
        "coreTestElement",
        "coreTestClockerComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Clock Benchmark Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "clock",      "Clock frequency", "1GHz" },
        { "clockcount", "Number of clock ticks to execute", "100000"}
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

    coreTestClockerComponent(SST::ComponentId_t id, SST::Params& params);
    void setup() {}
    void finish() {}

private:
    coreTestClockerComponent();                                // for serialization only
    coreTestClockerComponent(const coreTestClockerComponent&); // do not implement
    void operator=(const coreTestClockerComponent&);           // do not implement

    virtual bool tick(SST::Cycle_t);

    virtual bool Clock2Tick(SST::Cycle_t, uint32_t);
    virtual bool Clock3Tick(SST::Cycle_t, uint32_t);

    virtual void Oneshot1Callback(uint32_t);
    virtual void Oneshot2Callback();

    TimeConverter*      tc;
    Clock::HandlerBase* Clock3Handler;

    // Variables to store OneShot Callback Handlers
    OneShot::HandlerBase* callback1Handler;
    OneShot::HandlerBase* callback2Handler;

    std::string clock_frequency_str;
    int         clock_count;
};

} // namespace CoreTestClockerComponent
} // namespace SST

#endif // SST_CORE_CORETEST_CLOCKERCOMPONENT_H
