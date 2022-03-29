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

#ifndef SST_CORE_CORETEST_RNGCOMPONENT_H
#define SST_CORE_CORETEST_RNGCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/rng/sstrng.h"

using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace CoreTestRNGComponent {

class coreTestRNGComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestRNGComponent,
        "coreTestElement",
        "coreTestRNGComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Random number generation component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "seed_w",  "The seed to use for the random number generator", "7" },
        { "seed_z",  "The seed to use for the random number generator", "5" },
        { "seed",    "The seed to use for the random number generator.", "11" },
        { "rng",     "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
        { "count",   "The number of random numbers to generate, default is 1000", "1000" },
        { "verbose", "Sets the output verbosity of the component", "0" }
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

    coreTestRNGComponent(SST::ComponentId_t id, SST::Params& params);
    ~coreTestRNGComponent();
    void setup() {}
    void finish() {}

private:
    coreTestRNGComponent();                            // for serialization only
    coreTestRNGComponent(const coreTestRNGComponent&); // do not implement
    void operator=(const coreTestRNGComponent&);       // do not implement

    virtual bool tick(SST::Cycle_t);

    Output*     output;
    SSTRandom*  rng;
    std::string rng_type;
    int         rng_max_count;
    int         rng_count;
};

} // namespace CoreTestRNGComponent
} // namespace SST

#endif // SST_CORE_CORETEST_RNGCOMPONENT_H
