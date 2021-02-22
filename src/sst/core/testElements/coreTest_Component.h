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

#ifndef _CORETESTCOMPONENT_H
#define _CORETESTCOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

namespace SST {
namespace CoreTestComponent {

class coreTestComponent : public SST::Component
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestComponent,
        "coreTestElement",
        "coreTestComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CoreTest Test Component",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "workPerCycle", "Count of busy work to do during a clock tick.", NULL},
        { "commFreq",     "Approximate frequency of sending an event during a clock tick.", NULL},
        { "commSize",     "Size of communication to send.", "16"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"N", "events sent on N link", "counts", 1},
        {"S", "events sent on S link", "counts", 1},
        {"E", "events sent on E link", "counts", 1},
        {"W", "events sent on W link", "counts", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"Nlink", "Link to the coreTestComponent to the North", { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Slink", "Link to the coreTestComponent to the South", { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Elink", "Link to the coreTestComponent to the East",  { "coreTestComponent.coreTestComponentEvent", "" } },
        {"Wlink", "Link to the coreTestComponent to the West",  { "coreTestComponent.coreTestComponentEvent", "" } }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestComponent(SST::ComponentId_t id, SST::Params& params);
    ~coreTestComponent();

    void setup() { }
    void finish() {
    	printf("Component Finished.\n");
    }

private:
    coreTestComponent();  // for serialization only
    coreTestComponent(const coreTestComponent&); // do not implement
    void operator=(const coreTestComponent&); // do not implement

    void handleEvent(SST::Event *ev);
    virtual bool clockTic(SST::Cycle_t);

    int workPerCycle;
    int commFreq;
    int commSize;
    int neighbor;

    SST::RNG::MarsagliaRNG* rng;
    SST::Link* N;
    SST::Link* S;
    SST::Link* E;
    SST::Link* W;
    SST::Statistics::Statistic<int>* countN;
    SST::Statistics::Statistic<int>* countS;
    SST::Statistics::Statistic<int>* countE;
    SST::Statistics::Statistic<int>* countW;
};

} // namespace CoreTestComponent
} // namespace SST

#endif /* _CORETESTCOMPONENT_H */
