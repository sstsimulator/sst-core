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

//#include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_DistribComponent.h"

#include "sst/core/rng/discrete.h"
#include "sst/core/rng/expon.h"
#include "sst/core/rng/gaussian.h"
#include "sst/core/rng/poisson.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::CoreTestDistribComponent;

void
coreTestDistribComponent::finish()
{
    if ( bin_results ) {
        std::map<int64_t, uint64_t>::iterator map_itr;

        std::cout << "Bin:" << std::endl;
        for ( map_itr = bins->begin(); map_itr != bins->end(); map_itr++ ) {
            std::cout << map_itr->first << " " << map_itr->second << std::endl;
        }
    }
}

coreTestDistribComponent::coreTestDistribComponent(ComponentId_t id, Params& params) : Component(id)
{
    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    rng_max_count = params.find<int64_t>("count", 1000);
    rng_count     = 0;

    bins = new std::map<int64_t, uint64_t>();

    if ( "1" == params.find<std::string>("binresults", "1") ) { bin_results = true; }
    else {
        bin_results = false;
    }

    dist_type = params.find<std::string>("distrib", "gaussian");
    if ( "gaussian" == dist_type || "normal" == dist_type ) {
        double mean   = params.find<double>("mean", 1.0);
        double stddev = params.find<double>("stddev", 0.2);

        comp_distrib = new SSTGaussianDistribution(mean, stddev, new MersenneRNG(10111));
    }
    else if ( "exponential" == dist_type ) {
        double lambda = params.find<double>("lambda", 1.0);

        comp_distrib = new SSTExponentialDistribution(lambda, new MersenneRNG(10111));
    }
    else if ( "poisson" == dist_type ) {
        double lambda = params.find<double>("lambda", 3.0);

        comp_distrib = new SSTPoissonDistribution(lambda, new MersenneRNG(10111));
    }
    else if ( "discrete" == dist_type ) {
        uint32_t prob_count = (uint32_t)params.find<int64_t>("probcount", 1);

        double* probs = (double*)malloc(sizeof(double) * prob_count);

        printf("Will create discrete distribution with %" PRIu32 " probabilities.\n", prob_count);

        if ( 1 == prob_count ) { probs[0] = 1.0; }
        else {
            char* prob_name = (char*)malloc(sizeof(char) * 64);

            for ( uint32_t i = 0; i < prob_count; i++ ) {
                snprintf(prob_name, 64, "prob%" PRIu32, i);
                double prob_tmp = (double)params.find<double>(prob_name, 1.0 / (double)(prob_count));

                // printf("Probability at %" PRIu32 " : %f\n", i, prob_tmp);

                probs[i] = prob_tmp;
            }

            free(prob_name);

            probs[prob_count - 1] = 1.0;
        }

        comp_distrib = new SSTDiscreteDistribution(probs, prob_count, new MersenneRNG(10111));
    }
    else {
        std::cerr << "Unknown distribution type." << std::endl;
        exit(-1);
    }

    // set our clock
    registerClock("1GHz", new Clock::Handler<coreTestDistribComponent>(this, &coreTestDistribComponent::tick));
}

coreTestDistribComponent::coreTestDistribComponent() : Component(-1)
{
    // for serialization only
}

bool
coreTestDistribComponent::tick(Cycle_t UNUSED(cyc))
{
    double  next_result     = comp_distrib->getNextDouble();
    int64_t int_next_result = 0;

    if ( "discrete" == dist_type ) { int_next_result = (int64_t)(next_result * 100.0); }

    if ( bins->find(int_next_result) == bins->end() ) {
        bins->insert(std::pair<int64_t, uint64_t>(int_next_result, 1));
    }
    else {
        bins->at(int_next_result)++;
    }

    rng_count++;

    if ( rng_max_count == rng_count ) {
        primaryComponentOKToEndSim();
        return true;
    }

    return false;
}

// Element Library / Serialization stuff
