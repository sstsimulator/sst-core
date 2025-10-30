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

#ifndef SST_CORE_CORETEST_COMPONENT_EXT_H
#define SST_CORE_CORETEST_COMPONENT_EXT_H

#include "sst/core/component.h"
#include "sst/core/componentExtension.h"
#include "sst/core/link.h"
#include "sst/core/rng/marsaglia.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace SST::CoreTestComponent {

// Split the CoreTest_Component functionality into a component and extension
class coreTestComponentExt2 : public SST::ComponentExtension
{
public:
    coreTestComponentExt2(SST::ComponentId_t id, int neighbor);
    coreTestComponentExt2()  = default; // for serialization only
    ~coreTestComponentExt2() = default;

    void send(Event* ev);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestComponent::coreTestComponentExt2)

private:
    coreTestComponentExt2(const coreTestComponentExt2&)            = delete; // do not implement
    coreTestComponentExt2& operator=(const coreTestComponentExt2&) = delete; // do not implement

    void handleEvent(SST::Event* ev);

    int neighbor_;

    SST::Link*                       N_;
    SST::Link*                       S_;
    SST::Link*                       E_;
    SST::Link*                       W_;
    SST::Statistics::Statistic<int>* count_N_;
    SST::Statistics::Statistic<int>* count_S_;
    SST::Statistics::Statistic<int>* count_E_;
    SST::Statistics::Statistic<int>* count_W_;
};

class coreTestComponentExt : public SST::ComponentExtension
{
public:
    coreTestComponentExt(
        SST::ComponentId_t id, int64_t comm_freq, std::string clk, int64_t work_per_cycle, int64_t comm_size);
    ~coreTestComponentExt();

    int32_t generateNext();
    bool    communicate();

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestComponent::coreTestComponentExt)
    coreTestComponentExt() = default; // for serialization only

private:
    coreTestComponentExt(const coreTestComponentExt&)            = delete; // do not implement
    coreTestComponentExt& operator=(const coreTestComponentExt&) = delete; // do not implement

    bool clockTic(SST::Cycle_t);

    coreTestComponentExt2*  ext_;
    SST::RNG::MarsagliaRNG* rng_;
    int                     comm_freq_;
    int                     comm_size_;
    int                     work_per_cycle_;
};


class coreTestComponentExtMain : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestComponentExtMain,
        "coreTestElement",
        "coreTestComponentExtension",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CoreTest Test Component for ComponentExtensions",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "workPerCycle", "Count of busy work to do during a clock tick.", NULL},
        { "clockFrequency", "Frequency of the clock", "1GHz"},
        { "commFreq", "There is a 1/commFreq chance each clock cycle of sending an event to a neighbor", NULL},
        { "commSize",     "Size of communication to send.", "16"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "N", "events sent on N link", "counts", 1 },
        { "S", "events sent on S link", "counts", 1 },
        { "E", "events sent on E link", "counts", 1 },
        { "W", "events sent on W link", "counts", 1 }
    )

    SST_ELI_DOCUMENT_PORTS(
        {"Nlink", "Link to the coreTestComponentExtension to the North", { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Slink", "Link to the coreTestComponentExtension to the South", { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Elink", "Link to the coreTestComponentExtension to the East",  { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Wlink", "Link to the coreTestComponentExtension to the West",  { "coreTestComponent.coreTestComponentEvent", "" } }
    )

    SST_ELI_DOCUMENT_ATTRIBUTES(
        { "test_element", "true" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    SST_ELI_IS_CHECKPOINTABLE()

    coreTestComponentExtMain(SST::ComponentId_t id, SST::Params& params);
    ~coreTestComponentExtMain();

    void setup() override {}
    void finish() override { printf("Component Finished.\n"); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestComponent::coreTestComponentExtMain)
    coreTestComponentExtMain() = default; // for serialization only

private:
    coreTestComponentExtMain(const coreTestComponentExtMain&)            = delete; // do not implement
    coreTestComponentExtMain& operator=(const coreTestComponentExtMain&) = delete; // do not implement

    coreTestComponentExt* ext_;
};


} // namespace SST::CoreTestComponent

#endif // SST_CORE_CORETEST_COMPONENT_EXT_H
