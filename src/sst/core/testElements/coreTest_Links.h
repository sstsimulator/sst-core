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

#ifndef _CORETEST_LINKS_H
#define _CORETEST_LINKS_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

namespace SST {
namespace CoreTestComponent {

class coreTestLinks : public SST::Component
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestLinks,
        "coreTestElement",
        "coreTestLinks",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CoreTest Test Links",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "id",                 "ID of component", "" },
        { "added_send_latency", "Additional output latency to add to sends", "0ns"},
        { "added_recv_latency", "Additional input latency to add to incoming events", "0ns"},
        { "link_time_base",     "Timebase for links", "1ns" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS()

    SST_ELI_DOCUMENT_PORTS(
        {"Elink", "Link to the East",  { "NullEvent", "" } },
        {"Wlink", "Link to the West",  { "NullEvent", "" } }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestLinks(SST::ComponentId_t id, SST::Params& params);
    ~coreTestLinks();

    void setup() { }
    void finish() { }

private:

    int my_id;
    int recv_count;
    
    void handleEvent(SST::Event *ev, std::string from);
    virtual bool clockTic(SST::Cycle_t);

    SST::Link* E;
    SST::Link* W;
};

} // namespace CoreTestLinks
} // namespace SST

#endif /* _CORETEST_LINKS_H */
