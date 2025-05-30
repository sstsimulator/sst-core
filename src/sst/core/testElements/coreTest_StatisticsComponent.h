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

#ifndef SST_CORE_CORETEST_STATISTICSCOMPONENT_H
#define SST_CORE_CORETEST_STATISTICSCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/rng/rng.h"

#include <string>

using namespace SST;
using namespace SST::RNG;
using namespace SST::Statistics;

namespace SST::CoreTestStatisticsComponent {

class StatisticsComponentInt : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        StatisticsComponentInt,
        "coreTestElement",
        "StatisticsComponent.int",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Statistics test component with ints",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "seed_w", "The seed to use for the random number generator", "7" },
        { "seed_z", "The seed to use for the random number generator", "5" },
        { "seed", "The seed to use for the random number generator.", "11" },
        { "rng", "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
        { "count", "The number of random numbers to generate, default is 1000", "1000" },
        { "dynamic_reg", "The cycle at which to dynamically register a statistic. 0 indicates none", "0"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "stat1_U32", "Test Statistic 1 - Collecting U32 Data", "units", 1},
        { "stat2_U64", "Test Statistic 2 - Collecting U64 Data", "units", 2},
        { "stat3_I32", "Test Statistic 3 - Collecting I32 Data", "units", 3},
        { "stat4_I64", "Test Statistic 4 - Collecting I64 Data", "units", 4},
        { "stat5_dyn", "Test Statistic 5 - Statistic registered during run loop", "units", 1}
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    StatisticsComponentInt(ComponentId_t id, Params& params);
    StatisticsComponentInt(); // For checkpointing only
    void setup() override {}
    void finish() override {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(StatisticsComponentInt);

private:
    StatisticsComponentInt(const StatisticsComponentInt&)            = delete; // do not implement
    StatisticsComponentInt& operator=(const StatisticsComponentInt&) = delete; // do not implement

    virtual bool Clock1Tick(SST::Cycle_t);

    Random*     rng;
    std::string rng_type;
    int         rng_max_count;
    int         rng_count;
    int         dynamic_reg;
    Output&     output;

    // Statistics
    Statistic<uint32_t>* stat1_U32;
    Statistic<uint64_t>* stat2_U64;
    Statistic<int32_t>*  stat3_I32;
    Statistic<int64_t>*  stat4_I64;
    Statistic<int64_t>*  stat5_dyn;
};

class StatisticsComponentFloat : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        StatisticsComponentFloat,
        "coreTestElement",
        "StatisticsComponent.float",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Statistics test component with floats",
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
        { "stat1_F32", "Test Statistic 1 - Collecting F32 Data", "units", 1},
        { "stat2_F64", "Test Statistic 2 - Collecting F64 Data", "units", 2},
        { "stat3_F64", "Test Statistic 2 - Collecting F64 Data", "units", 9},
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    StatisticsComponentFloat(ComponentId_t id, Params& params);
    StatisticsComponentFloat(); // For serialization only
    void setup() override {}
    void finish() override {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(StatisticsComponentFloat);

private:
    StatisticsComponentFloat(const StatisticsComponentFloat&) = delete; // do not implement
    void operator=(const StatisticsComponentFloat&)           = delete; // do not implement

    virtual bool Clock1Tick(SST::Cycle_t);

    Random*     rng;
    std::string rng_type;
    int         rng_max_count;
    int         rng_count;
    Output&     output;

    // Statistics
    Statistic<float>*  stat1_F32;
    Statistic<double>* stat2_F64;
    Statistic<double>* stat3_F64; // For testing stat sharing
};

} // namespace SST::CoreTestStatisticsComponent

#endif // SST_CORE_CORETEST_STATISTICSCOMPONENT_H
