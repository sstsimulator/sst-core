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

#include "sst_config.h"

#include "sst/core/testElements/coreTest_StatisticsComponent.h"

#include "sst/core/rng/marsaglia.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/simulation.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::CoreTestStatisticsComponent;

StatisticsComponentInt::StatisticsComponentInt(ComponentId_t id, Params& params) :
    Component(id),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Get runtime parameters from the input file (.py),
    // these will set the random number generation.
    rng_count     = 0;
    rng_max_count = params.find<int64_t>("count", 1000);

    std::string rngType = params.find<std::string>("rng", "mersenne");

    if ( rngType == "mersenne" ) {
        unsigned int seed = params.find<int64_t>("seed", 1447);

        output.output("Using Mersenne Random Number Generator with seed = %u\n", seed);
        rng = new MersenneRNG(seed);
    }
    else if ( rngType == "marsaglia" ) {
        unsigned int m_w = params.find<int64_t>("seed_w", 0);
        unsigned int m_z = params.find<int64_t>("seed_z", 0);

        if ( m_w == 0 || m_z == 0 ) {
            output.output("Using Marsaglia Random Number Generator with no seeds ...\n");
            rng = new MarsagliaRNG();
        }
        else {
            output.output("Using Marsaglia Random Number Generator with seeds m_z = %u, m_w = %u\n", m_z, m_w);
            rng = new MarsagliaRNG(m_z, m_w);
        }
    }
    else {
        output.output("RNG provided but unknown %s, so using Mersenne with seed = 1447...\n", rngType.c_str());
        rng = new MersenneRNG(1447);
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    /////////////////////////////////////////
    // Register the Clock

    // First 1ns Clock
    output.output("REGISTER CLOCK #1 at 1 ns\n");
    registerClock("1 ns", new Clock::Handler<StatisticsComponentInt>(this, &StatisticsComponentInt::Clock1Tick));

    /////////////////////////////////////////
    // Create the Statistics objects
    stat1_U32 = registerStatistic<uint32_t>("stat1_U32", "1");
    stat2_U64 = registerStatistic<uint64_t>("stat2_U64", "2");
    stat3_I32 = registerStatistic<int32_t>("stat3_I32", "3");
    stat4_I64 = registerStatistic<int64_t>("stat4_I64", "4");

    // Try to Register A duplicate Statistic Name
    auto stat_NOTUSED =
        registerStatistic<uint32_t>("stat1_U32", "1"); // This registration will return the previously registered stat

    // Check to see that we got the same stat as before, unless this
    // is a NullStatistic, in which case it won't be the same
    if ( stat1_U32 != stat_NOTUSED && !stat_NOTUSED->isNullStatistic() ) {
        output.output("ERROR: When reregistering the same statistic, did not recieve the same object back\n");
    }

    // Test Statistic functions for delayed output and collection and to disable Stat
    //    stat1_U32->disable();
    //    stat1_U32->delayOutput("10 ns");
    //    stat1_U32->delayCollection("10 ns");
}

StatisticsComponentInt::StatisticsComponentInt() :
    Component(-1),
    output(Simulation::getSimulation()->getSimulationOutput())
{}

bool
StatisticsComponentInt::Clock1Tick(Cycle_t UNUSED(CycleNum))
{
    // NOTE: THIS IS THE 1NS CLOCK
    //    std::cout << "@ " << CycleNum << std::endl;

    uint32_t U32 = rng->generateNextUInt32();
    uint64_t U64 = rng->generateNextUInt64();
    int32_t  I32 = rng->generateNextInt32();
    int64_t  I64 = rng->generateNextInt64();
    rng_count++;

    // Scale the data
    uint32_t scaled_U32 = U32 / 10000000;
    uint64_t scaled_U64 = U64 / 1000000000000000;
    int32_t  scaled_I32 = I32 / 10000000;
    int64_t  scaled_I64 = I64 / 1000000000000000;

    // Add the Statistic Data
    stat1_U32->addData(scaled_U32);
    stat2_U64->addData(scaled_U64);
    stat3_I32->addData(scaled_I32);
    stat4_I64->addData(scaled_I64);

    // return false so we keep going or true to stop
    if ( rng_count >= rng_max_count ) {
        primaryComponentOKToEndSim();
        return true;
    }
    else {
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

StatisticsComponentFloat::StatisticsComponentFloat(ComponentId_t id, Params& params) :
    Component(id),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Get runtime parameters from the input file (.py),
    // these will set the random number generation.
    rng_count     = 0;
    rng_max_count = params.find<int64_t>("count", 1000);

    std::string rngType = params.find<std::string>("rng", "mersenne");

    if ( rngType == "mersenne" ) {
        unsigned int seed = params.find<int64_t>("seed", 1447);

        output.output("Using Mersenne Random Number Generator with seed = %u\n", seed);
        rng = new MersenneRNG(seed);
    }
    else if ( rngType == "marsaglia" ) {
        unsigned int m_w = params.find<int64_t>("seed_w", 0);
        unsigned int m_z = params.find<int64_t>("seed_z", 0);

        if ( m_w == 0 || m_z == 0 ) {
            output.output("Using Marsaglia Random Number Generator with no seeds ...\n");
            rng = new MarsagliaRNG();
        }
        else {
            output.output("Using Marsaglia Random Number Generator with seeds m_z = %u, m_w = %u\n", m_z, m_w);
            rng = new MarsagliaRNG(m_z, m_w);
        }
    }
    else {
        output.output("RNG provided but unknown %s, so using Mersenne with seed = 1447...\n", rngType.c_str());
        rng = new MersenneRNG(1447);
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    /////////////////////////////////////////
    // Register the Clock

    // First 1ns Clock
    output.output("REGISTER CLOCK #1 at 1 ns\n");
    registerClock("1 ns", new Clock::Handler<StatisticsComponentFloat>(this, &StatisticsComponentFloat::Clock1Tick));

    /////////////////////////////////////////
    // Create the Statistics objects
    stat1_F32 = registerStatistic<float>("stat1_F32", "1");
    stat2_F64 = registerStatistic<double>("stat2_F64", "2");
    stat3_F64 = registerStatistic<double>("stat3_F64", "3");
}

StatisticsComponentFloat::StatisticsComponentFloat() :
    Component(-1),
    output(Simulation::getSimulation()->getSimulationOutput())
{}

bool
StatisticsComponentFloat::Clock1Tick(Cycle_t UNUSED(CycleNum))
{

    double value_F32 = rng->nextUniform();
    double value_F64 = rng->nextUniform();
    rng_count++;

    // Scale the data
    float  scaled_F32 = (float)(value_F32 * 1000);
    double scaled_F64 = value_F64 * 1000;


    // Add the Statistic Data
    stat1_F32->addData(scaled_F32);
    stat2_F64->addData(scaled_F64);
    stat3_F64->addData(scaled_F64 + 10.0);

    ////////////////////////////////////////////////////////

    // return false so we keep going or true to stop
    if ( rng_count >= rng_max_count ) {
        primaryComponentOKToEndSim();
        return true;
    }
    else {
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
