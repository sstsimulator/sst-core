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

#ifndef SST_CORE_CORETEST_DISTRIBCOMPONENT_H
#define SST_CORE_CORETEST_DISTRIBCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/rng/distrib.h"

using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace CoreTestDistribComponent {

class coreTestDistribComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestDistribComponent,
        "coreTestElement",
        "coreTestDistribComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Random Number Distribution Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "count",             "Number of random values to generate from the distribution", "1000"},
        { "distrib",           "Random distribution to use - \"gaussian\" (or \"normal\"), or \"exponential\"", "gaussian"},
        { "mean",              "Mean value to use if we are sampling from the Gaussian/Normal distribution", "1.0"},
        { "stddev",            "Standard deviation to use for the distribution", "0.2"},
        { "lambda",            "Lambda value to use for the exponential distribution", "1.0"},
        { "binresults",        "Print the results, only if value is \"1\"", "1"},
        { "probcount",         "Number of probabilities in discrete distribution", "1"},
        { "prob%(probcount)d", "Probability values for discrete distribution", "1"}
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

    coreTestDistribComponent(SST::ComponentId_t id, SST::Params& params);
    void finish();
    void setup() {}

private:
    coreTestDistribComponent();                                // for serialization only
    coreTestDistribComponent(const coreTestDistribComponent&); // do not implement
    void operator=(const coreTestDistribComponent&);           // do not implement

    virtual bool tick(SST::Cycle_t);

    SSTRandomDistribution* comp_distrib;

    int         rng_max_count;
    int         rng_count;
    bool        bin_results;
    std::string dist_type;

    std::map<int64_t, uint64_t>* bins;
};

} // namespace CoreTestDistribComponent
} // namespace SST

#endif // SST_CORE_CORETEST_DISTRIBCOMPONENT_H
