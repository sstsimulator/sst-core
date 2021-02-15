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

#ifndef _CORETESTSTATISTICSCOMPONENT_H
#define _CORETESTSTATISTICSCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/rng/sstrng.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::Statistics;

namespace SST {
namespace CoreTestStatisticsComponent {

class coreTestStatisticsComponent : public SST::Component
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestStatisticsComponent,
        "coreTestElement",
        "coreTestStatisticsComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Statistics Demo Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "seed_w", "The seed to use for the random number generator", "7" },
        { "seed_z", "The seed to use for the random number generator", "5" },
        { "seed", "The seed to use for the random number generator.", "11" },
        { "rng", "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
        { "count", "The number of random numbers to generate, default is 1000", "1000" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "stat1_U32", "Test Statistic 1 - Collecting U32 Data", "units", 1},
        { "stat2_U64", "Test Statistic 2 - Collecting U64 Data", "units", 2},
        { "stat3_I32", "Test Statistic 3 - Collecting I32 Data", "units", 3},
        { "stat4_I64", "Test Statistic 4 - Collecting I64 Data", "units", 4},
        { "stat5_U32", "Test Statistic 5 - Collecting U32 Data", "units", 5},
        { "stat6_U64", "Test Statistic 6 - Collecting U64 Data", "units", 6}
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestStatisticsComponent(ComponentId_t id, Params& params);
    void setup()  { }
    void finish() { }

private:
    coreTestStatisticsComponent();  // for serialization only
    coreTestStatisticsComponent(const coreTestStatisticsComponent&); // do not implement
    void operator=(const coreTestStatisticsComponent&); // do not implement

    virtual bool Clock1Tick(SST::Cycle_t);

    SSTRandom* rng;
    std::string rng_type;
    int rng_max_count;
    int rng_count;
    Output& output;

    // Histogram Statistics
    Statistic<uint32_t>*  stat1_U32;
    Statistic<uint64_t>*  stat2_U64;
    Statistic<int32_t>*   stat3_I32;
    Statistic<int64_t>*   stat4_I64;

    // Accumulator Statistics
    Statistic<uint32_t>*  stat5_U32;
    Statistic<uint64_t>*  stat6_U64;
    Statistic<uint32_t>*  stat7_U32_NOTUSED;

};

} // namespace CoreTestStatistics
} // namespace SST

#endif /* _CORETESTSTATISTICSCOMPONENT_H */
