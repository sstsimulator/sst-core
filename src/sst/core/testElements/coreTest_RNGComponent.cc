// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_RNGComponent.h"

#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/marsaglia.h"
#include "sst/core/rng/xorshift.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::CoreTestRNGComponent;

coreTestRNGComponent::coreTestRNGComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    rng_count = 0;
    rng_max_count = params.find<int64_t>("count", 1000);

    uint32_t verbose = (uint32_t) params.find<int64_t>("verbose", 0);
    output = new Output("RNGComponent", verbose, 0, Output::STDOUT);

    std::string rngType = params.find<std::string>("rng", "mersenne");

    if (rngType == "mersenne") {
        const uint32_t seed = (uint32_t) params.find<int64_t>("seed", 1447);

	output->verbose(CALL_INFO, 1, 0, "Using Mersenne Generator with seed: %" PRIu32 "\n", seed);
        rng = new MersenneRNG(seed);
    } else if (rngType == "marsaglia") {
        const uint32_t m_w = (uint32_t) params.find<int64_t>("seed_w", 0);
        const uint32_t m_z = (uint32_t) params.find<int64_t>("seed_z", 0);

        if(m_w == 0 || m_z == 0) {
	    output->verbose(CALL_INFO, 1, 0, "Using Marsaglia Generator with no seeds...\n");
            rng = new MarsagliaRNG();
        } else {
	    output->verbose(CALL_INFO, 1, 0, "Using Marsaglia Generator with seeds: Z=%" PRIu32 ", W=%" PRIu32 "\n",
		m_w, m_z);
            rng = new MarsagliaRNG(m_z, m_w);
        }
    } else if (rngType == "xorshift") {
	uint32_t seed = (uint32_t) params.find<int64_t>("seed", 57);
	output->verbose(CALL_INFO, 1, 0, "Using XORShift Generator with seed: %" PRIu32 "\n", seed);
	rng = new XORShiftRNG(seed);
    } else {
	output->verbose(CALL_INFO, 1, 0, "Generator: %s is unknown, using Mersenne with standard seed\n",
		rngType.c_str());
        rng = new MersenneRNG(1447);
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    registerClock("1GHz", new Clock::Handler<coreTestRNGComponent>(this,
			       &coreTestRNGComponent::tick));
}

coreTestRNGComponent::~coreTestRNGComponent() {
	delete output;
}

coreTestRNGComponent::coreTestRNGComponent() :
    Component(-1)
{
    // for serialization only
}

bool coreTestRNGComponent::tick( Cycle_t )
{
    double nU = rng->nextUniform();
    uint32_t U32 = rng->generateNextUInt32();
    uint64_t U64 = rng->generateNextUInt64();
    int32_t I32 = rng->generateNextInt32();
    int64_t I64 = rng->generateNextInt64();
    rng_count++;

    output->verbose(CALL_INFO, 1, 0, "Random: %" PRIu32 " of %" PRIu32 " %18.15f %" PRIu32 ", %" PRIu64 ", %" PRId32 ", %" PRId64 "\n",
	rng_count, rng_max_count, nU, U32, U64, I32, I64);

   // return false so we keep going
  	if(rng_count == rng_max_count) {
  	    primaryComponentOKToEndSim();
  		return true;
  	} else {
  		return false;
  	}
}

// Element Library / Serialization stuff


