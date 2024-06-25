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

#ifndef SST_CORE_CORETEST_MODULE_H
#define SST_CORE_CORETEST_MODULE_H

#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/module.h"
#include "sst/core/rng/rng.h"

#include <vector>

using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace CoreTestModule {

class CoreTestModuleExample : public SST::Module
{

public:
    SST_ELI_REGISTER_MODULE_API(SST::CoreTestModule::CoreTestModuleExample) // API & module in one

    SST_ELI_REGISTER_MODULE(
        CoreTestModuleExample, "coreTestElement", "CoreTestModule", SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "CoreTest module to demonstrate interface.", SST::CoreTestModule::CoreTestModuleExample)

    SST_ELI_DOCUMENT_PARAMS(
        { "rng",     "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
        { "seed_w",  "The seed to use for the random number generator", "7" },
        { "seed_z",  "The seed to use for the random number generator", "5" },
        { "seed",    "The seed to use for the random number generator.", "11" },
    )

    CoreTestModuleExample(SST::Params& params);
    ~CoreTestModuleExample();
    std::string getRNGType() const;
    uint32_t    getNext();

    CoreTestModuleExample() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestModule::CoreTestModuleExample)

private:
    std::string rng_type;
    Random*     rng;
};


class coreTestModuleLoader : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestModuleLoader,
        "coreTestElement",
        "coreTestModuleLoader",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Component that loads an RNG module",
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

    coreTestModuleLoader(SST::ComponentId_t id, SST::Params& params);
    ~coreTestModuleLoader();
    void setup() override;
    void finish() override;
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestModule::coreTestModuleLoader)

private:
    coreTestModuleLoader(); // for serialization only

    virtual bool tick(SST::Cycle_t);

    Output*                output;
    int                    rng_max_count;
    int                    rng_count;
    CoreTestModuleExample* rng_module;
};

} // namespace CoreTestModule
} // namespace SST

#endif // SST_CORE_CORETEST_MODULE_H
