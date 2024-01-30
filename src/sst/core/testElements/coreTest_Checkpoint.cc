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

#include "sst/core/testElements/coreTest_Checkpoint.h"

#include "sst/core/rng/constant.h"
#include "sst/core/rng/discrete.h"
#include "sst/core/rng/expon.h"
#include "sst/core/rng/gaussian.h"
#include "sst/core/rng/marsaglia.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/poisson.h"
#include "sst/core/rng/uniform.h"
#include "sst/core/rng/xorshift.h"

#include <assert.h>

using namespace SST;
using namespace SST::CoreTestCheckpoint;

coreTestCheckpoint::coreTestCheckpoint(ComponentId_t id, Params& params) : Component(id)
{
    bool starter = params.find<bool>("starter", true);
    if ( starter ) { counter = params.find<uint32_t>("counter", 1000); }
    else {
        counter = 0;
    }
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    link = configureLink("port", new Event::Handler2<coreTestCheckpoint, &coreTestCheckpoint::handleEvent>(this));
    sst_assert(link, CALL_INFO, -1, "Could not configure link");

    test_string = params.find<std::string>("test_string", "");

    std::string freq = params.find<std::string>("clock_frequency", "100kHz");

    // Need to keep a pointer to the clock handler so we can
    // reregister clock
    clock_handler = new Clock::Handler2<coreTestCheckpoint, &coreTestCheckpoint::handleClock>(this);
    clock_tc      = registerClock(freq, clock_handler);

    duty_cycle       = params.find<int>("clock_duty_cycle", 10);
    duty_cycle_count = duty_cycle;

    self_link = configureSelfLink(
        "clock_restart", clock_tc, new Event::Handler2<coreTestCheckpoint, &coreTestCheckpoint::restartClock>(this));

    // Output
    output = new Output(
        params.find<std::string>("output_prefix", ""), params.find<uint32_t>("output_verbose", 0), 0,
        Output::output_location_t::STDOUT);

    // RNG & Distributions
    marsaglia =
        new RNG::MarsagliaRNG(params.find<unsigned int>("rng_seed_w", 7), params.find<unsigned int>("rng_seed_z", 5));
    mersenne = new RNG::MersenneRNG(params.find<unsigned int>("rng_seed", 11));
    xorshift = new RNG::XORShiftRNG(params.find<unsigned int>("rng_seed", 11));

    dist_const = new RNG::ConstantDistribution(params.find<double>("dist_const", 1.5));

    std::vector<double> discrete_probs;
    params.find_array<double>("dist_discrete_probs", discrete_probs);
    if ( discrete_probs.empty() ) discrete_probs.push_back(1.0);

    dist_discrete = new RNG::DiscreteDistribution(discrete_probs.data(), discrete_probs.size());

    dist_expon = new RNG::ExponentialDistribution(params.find<double>("dist_exp_lambda", 1.0));

    dist_gauss = new RNG::GaussianDistribution(
        params.find<double>("dist_gauss_mean", 1.0), params.find<double>("dist_gauss_stddev", 0.2));

    dist_poisson = new RNG::PoissonDistribution(params.find<double>("dist_poisson_lambda", 1.0));

    dist_uniform = new RNG::UniformDistribution(params.find<uint32_t>("dist_uni_bins", 4));
}

coreTestCheckpoint::~coreTestCheckpoint() {}

void
coreTestCheckpoint::setup()
{
    if ( counter > 0 ) link->send(new coreTestCheckpointEvent(counter));
}

// Report state that should persist through checkpoint/restart
void
coreTestCheckpoint::finish()
{
    output->output(
        "%s finished. teststring=%s, output=('%s',%" PRIu32 ")\n", getName().c_str(), test_string.c_str(),
        output->getPrefix().c_str(), output->getVerboseLevel());
}

// incoming event is bounced back after decrementing its counter
// if counter is 0, end simulation
void
coreTestCheckpoint::handleEvent(Event* ev)
{
    coreTestCheckpointEvent* event = static_cast<coreTestCheckpointEvent*>(ev);

    if ( event->decCount() ) {
        getSimulationOutput().output("%s, OK to end simulation\n", getName().c_str());
        primaryComponentOKToEndSim();
    }
    getSimulationOutput().output(
        "%s, bounce %d, t=%" PRIu64 "\n", getName().c_str(), event->getCount(), getCurrentSimCycle());
    link->send(event);
}

// clock hander just prints
bool
coreTestCheckpoint::handleClock(Cycle_t cycle)
{
    getSimulationOutput().output("Clock cycle count = %" PRIu64 "\n", cycle);
    output->output(
        "RNG: %" PRIu32 ", %" PRIu32 ", %" PRIu32 "\n", marsaglia->generateNextUInt32(), mersenne->generateNextUInt32(),
        xorshift->generateNextUInt32());
    output->output(
        "Distributions: %f, %f, %f, %f, %f, %f\n", dist_const->getNextDouble(), dist_discrete->getNextDouble(),
        dist_expon->getNextDouble(), dist_gauss->getNextDouble(), dist_poisson->getNextDouble(),
        dist_uniform->getNextDouble());
    duty_cycle_count--;
    if ( duty_cycle_count == 0 ) {
        duty_cycle_count = duty_cycle;
        // Send a wakeup
        self_link->send(duty_cycle, nullptr);
        return true;
    }
    return false;
}

// restarts the clock
void
coreTestCheckpoint::restartClock(Event* UNUSED(ev))
{
    // Event passed in is nullptr, no need to do anything with it
    reregisterClock(clock_tc, clock_handler);
}


void
coreTestCheckpoint::printStatus(Output& out)
{
    out.output("Component Status: %s, %p, %" PRIu32 ", %s\n", getName().c_str(), link, counter, test_string.c_str());
}


void
coreTestCheckpoint::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::Component::serialize_order(ser);
    SST_SER(link)
    SST_SER(self_link)
    SST_SER(clock_handler)
    SST_SER(clock_tc)
    SST_SER(duty_cycle)
    SST_SER(duty_cycle_count)
    SST_SER(counter)
    SST_SER(test_string); // Make sure semicolon is a no-op
    SST_SER(counter);
    SST_SER(test_string);
    SST_SER(output);
    SST_SER(mersenne)
    SST_SER(marsaglia)
    SST_SER(xorshift)
    SST_SER(dist_const)
    SST_SER(dist_discrete)
    SST_SER(dist_expon)
    SST_SER(dist_gauss)
    SST_SER(dist_poisson)
    SST_SER(dist_uniform)
}


// Element Libarary / Serialization stuff
