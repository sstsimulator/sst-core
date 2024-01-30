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

#include "sst_config.h"

#include "sst/core/testElements/coreTest_Module.h"

#include "sst/core/rng/marsaglia.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/xorshift.h"

using namespace SST::CoreTestModule;
using namespace SST::RNG;

CoreTestModuleExample::CoreTestModuleExample(SST::Params& params)
{
    rng_type = params.find<std::string>("rng", "mersenne");
    if ( rng_type == "mersenne" ) {
        const uint32_t seed = (uint32_t)params.find<int64_t>("seed", 1447);
        rng                 = new SST::RNG::MersenneRNG(seed);
    }
    else if ( rng_type == "marsaglia" ) {
        const uint32_t m_w = (uint32_t)params.find<int64_t>("seed_w", 0);
        const uint32_t m_z = (uint32_t)params.find<int64_t>("seed_z", 0);

        if ( m_w == 0 || m_z == 0 ) { rng = new MarsagliaRNG(); }
        else {
            rng = new MarsagliaRNG(m_z, m_w);
        }
    }
    else if ( rng_type == "xorshift" ) {
        uint32_t seed = (uint32_t)params.find<int64_t>("seed", 57);
        rng           = new XORShiftRNG(seed);
    }
    else {
        rng = new MersenneRNG(1447);
    }
}

CoreTestModuleExample::~CoreTestModuleExample()
{
    delete rng;
}

std::string
CoreTestModuleExample::getRNGType() const
{
    return rng_type;
}

uint32_t
CoreTestModuleExample::getNext()
{
    return rng->generateNextUInt32();
}

void
CoreTestModuleExample::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::Module::serialize_order(ser);
    ser& rng_type;
    ser& rng;
}


coreTestModuleLoader::coreTestModuleLoader(SST::ComponentId_t id, SST::Params& params) : Component(id)
{
    rng_count     = 0;
    rng_max_count = params.find<int64_t>("count", 1000);

    uint32_t verbose = (uint32_t)params.find<int64_t>("verbose", 0);
    output           = new Output("RNGComponent", verbose, 0, Output::STDOUT);

    std::string rngType = params.find<std::string>("rng", "mersenne");

    Params      moduleParams;
    std::string defaultSeed = (rngType == "mersenne") ? "1447" : "57";
    moduleParams.insert("seed", params.find<std::string>("seed", defaultSeed));
    moduleParams.insert("seed_w", params.find<std::string>("seed_w", "0"));
    moduleParams.insert("seed_z", params.find<std::string>("seed_z", "0"));

    if ( rngType == "mersenne" || rngType == "xorshift" ) {
        output->verbose(
            CALL_INFO, 1, 0, "Using %s Generator with seed: %s\n", rngType.c_str(),
            moduleParams.find<std::string>("seed").c_str());
    }
    else if ( rngType == "marsaglia" ) {
        output->verbose(
            CALL_INFO, 1, 0, "Using Marsaglia Generator with seeds: Z=%s, W=%s\n",
            moduleParams.find<std::string>("seed_w").c_str(), moduleParams.find<std::string>("seed_z").c_str());
    }
    else {
        output->verbose(
            CALL_INFO, 1, 0, "Generator: %s is unknown, using Mersenne with standard seed\n", rngType.c_str());
    }


    rng_module = loadModule<CoreTestModuleExample>("coreTestElement.CoreTestModule", moduleParams);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // set our clock
    registerClock("1GHz", new Clock::Handler2<coreTestModuleLoader, &coreTestModuleLoader::tick>(this));
}

coreTestModuleLoader::~coreTestModuleLoader()
{
    delete output;
}

void
coreTestModuleLoader::setup()
{}

void
coreTestModuleLoader::finish()
{}

coreTestModuleLoader::coreTestModuleLoader() : Component()
{ /* For serialization ONLY*/
}

bool coreTestModuleLoader::tick(SST::Cycle_t)
{
    uint32_t next = rng_module->getNext();
    rng_count++;

    output->verbose(
        CALL_INFO, 1, 0, "Random: %" PRIu32 " of %" PRIu32 ": %" PRIu32 "\n", rng_count, rng_max_count, next);

    // return false so we keep going
    if ( rng_count == rng_max_count ) {
        primaryComponentOKToEndSim();
        return true;
    }
    else {
        return false;
    }
}

void
coreTestModuleLoader::serialize_order(SST::Core::Serialization::serializer& ser)
{
    Component::serialize_order(ser);
    SST_SER(output)
    SST_SER(rng_max_count)
    SST_SER(rng_count)
    SST_SER(rng_module)
}