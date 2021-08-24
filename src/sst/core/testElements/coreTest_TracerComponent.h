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

#ifndef SST_CORE_CORETEST_TRACERCOMPONENT_H
#define SST_CORE_CORETEST_TRACERCOMPONENT_H

#include <sst/elements/memHierarchy/memEvent.h>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

namespace SST {
namespace CoreTestTracerComponent {

class coreTestTracerComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestRNGComponent,
        "coreTestElement",
        "coreTestRNGComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Random number generation component",
        COMPONENT_CATEGORY_UNCATEGORIZED)

    SST_ELI_DOCUMENT_PARAMS(
        { "seed_w", "The seed to use for the random number generator", "7" },
        { "seed_z", "The seed to use for the random number generator", "5" },
        { "seed", "The seed to use for the random number generator.", "11" },
        { "rng", "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
        { "count", "The number of random numbers to generate, default is 1000", "1000" },
        { "verbose", "Sets the output verbosity of the component", "0" },
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )

    coreTestTracerComponent(SST::ComponentId_t id, Params& params);
    ~coreTestTracerComponent();
    void finish();

private:
    // Functions
    bool clock(SST::Cycle_t);
    void FinalStats(FILE*, unsigned int);
    void PrintAddrHistogram(FILE*, vector<SST::MemHierarchy::Addr>);
    void PrintAccessLatencyDistribution(FILE*, unsigned int);

    Output* out;
    FILE*   traceFile;
    FILE*   statsFile;

    // Links
    SST::Link* northBus;
    SST::Link* southBus;

    // Input Parameters
    unsigned int stats;
    unsigned int pageSize;
    unsigned int accessLatBins;

    // Flags
    bool writeTrace;
    bool writeStats;
    bool writeDebug_8;

    unsigned int nbCount;
    unsigned int sbCount;
    uint64_t     timestamp;

    vector<SST::MemHierarchy::Addr> AddrHist; // Address Histogram
    vector<unsigned int>            AccessLatencyDist;

    map<MemEvent::id_type, uint64_t> InFlightReqQueue;

    TimeConverter* picoTimeConv;
    TimeConverter* nanoTimeConv;

    // Serialization
    coreTestTracerComponent();                               // for serialization only
    coreTestTracerComponent(const coreTestTracerComponent&); // do not implement
    void operator=(const coreTestTracerComponent&);          // do not implement

    // static const ElementInfoParam coreTestRNGComponent_params[] = {
    //    { "seed_w", "The seed to use for the random number generator", "7" },
    //    { "seed_z", "The seed to use for the random number generator", "5" },
    //    { "seed", "The seed to use for the random number generator.", "11" },
    //    { "rng", "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
    //    { "count", "The number of random numbers to generate, default is 1000", "1000" },
    //    { "verbose", "Sets the output verbosity of the component", "0" },
    //    { NULL, NULL, NULL }
    //};

    // static const ElementInfoComponent coreTestElementComponents[] = {
    //    { "coreTestRNGComponent",                              // Name
    //      "Random number generation component",              // Description
    //      NULL,                                              // PrintHelp
    //      create_coreTestRNGComponent,                         // Allocator
    //      coreTestRNGComponent_params,                         // Parameters
    //      NULL,                                              // Ports
    //      COMPONENT_CATEGORY_UNCATEGORIZED,                  // Category
    //      NULL                                               // Statistics
    //    },
}; // class coreTestTracerComponent

} // namespace CoreTestTracerComponent
} // namespace SST

#endif // SST_CORE_CORETEST_TRACERCOMPONENT_H
