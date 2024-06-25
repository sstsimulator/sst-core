// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_CHECKPOINT_H
#define SST_CORE_CORETEST_CHECKPOINT_H

#include "sst/core/component.h"
#include "sst/core/event.h"
#include "sst/core/link.h"
#include "sst/core/rng/distrib.h"
#include "sst/core/rng/rng.h"

namespace SST {
namespace CoreTestCheckpoint {

// Very simple starting case
// Expected to have two components in simulation.
// The components ping-pong an event until its count reaches 0

class coreTestCheckpointEvent : public SST::Event
{
public:
    coreTestCheckpointEvent() : SST::Event(), counter(1000) {}

    coreTestCheckpointEvent(uint32_t c) : SST::Event(), counter(c) {}

    ~coreTestCheckpointEvent() {}

    bool decCount()
    {
        if ( counter != 0 ) counter--;
        return counter == 0;
    }

    uint32_t getCount() { return counter; }

private:
    uint32_t counter;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        SST_SER(counter)
    }

    ImplementSerializable(SST::CoreTestCheckpoint::coreTestCheckpointEvent);
};


class coreTestCheckpoint : public SST::Component
{
public:
    SST_ELI_REGISTER_COMPONENT(
        coreTestCheckpoint,
        "coreTestElement",
        "coreTestCheckpoint",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CoreTest Test Checkpoint",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "starter", "Whether this component initiates the ping-pong", "T"},
        { "count", "Number of times to bounce the message back and forth", "1000" },
        { "test_string", "A test string", ""},
        { "clock_frequency", "Frequency for clock", "100kHz"},
        { "clock_duty_cycle", "Number of cycles to keep clock on and off", "10"},
        // Testing output options
        { "output_prefix", "Prefix for output", ""},
        { "output_verbose", "Verbosity for output", "0"},
        // Testing RNG & distributions
        { "rng_seed_w",        "The first seed for marsaglia", "7" },
        { "rng_seed_z",        "The second seed for marsaglia", "5" },
        { "rng_seed",          "The seed for mersenne and xorshift", "11" },
        { "dist_const",        "Constant for ConstantDistribution", "1.5" },
        { "dist_discrete_probs", "Probabilities in discrete distribution", "[1.0]"},
        { "dist_exp_lambda",    "Lambda for exponentional distribution", "1.0"},
        { "dist_gauss_mean",    "Mean for Gaussian distribution", "1.0"},
        { "dist_gauss_stddev",  "Standard deviation for Gaussian distribution", "0.2"},
        { "dist_poisson_lambda", "Lambda for Poisson distribution", "1.0"},
        { "dist_uni_bins",      "Number of proability bins for the uniform distribution", "4"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"port", "Link to the other coreTestCheckpoint", { "coreTestElement.coreTestCheckpointEvent", "" } }
    )

    coreTestCheckpoint(ComponentId_t id, SST::Params& params);
    ~coreTestCheckpoint();

    void setup() override;

    void finish() override;

    void printStatus(Output& out) override;

    // Serialization functions and macro
    coreTestCheckpoint() : Component() {} // For serialization only
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestCheckpoint::coreTestCheckpoint)

private:
    void handleEvent(SST::Event* ev);
    bool handleClock(Cycle_t cycle);
    void restartClock(SST::Event* ev);

    SST::Link*          link;
    SST::Link*          self_link;
    TimeConverter*      clock_tc;
    Clock::HandlerBase* clock_handler;
    int                 duty_cycle;       // Used to count clock on and off cycles
    int                 duty_cycle_count; // Used to count clock on and off cycles
    uint32_t            counter;          // Unused after setup
    std::string         test_string;      // Test that string got serialized
    Output*             output;

    RNG::Random*             mersenne;
    RNG::Random*             marsaglia;
    RNG::Random*             xorshift;
    RNG::RandomDistribution* dist_const;
    RNG::RandomDistribution* dist_discrete;
    RNG::RandomDistribution* dist_expon;
    RNG::RandomDistribution* dist_gauss;
    RNG::RandomDistribution* dist_poisson;
    RNG::RandomDistribution* dist_uniform;
};

} // namespace CoreTestCheckpoint
} // namespace SST

#endif // SST_CORE_CORETEST_CHECKPOINT_H
