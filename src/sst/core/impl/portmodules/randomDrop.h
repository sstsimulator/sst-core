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

#ifndef SST_CORE_IMPL_PORTMODULE_RANDOMDROP_H
#define SST_CORE_IMPL_PORTMODULE_RANDOMDROP_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/portModule.h"
#include "sst/core/threadsafe.h"
#include "sst/core/timeVortex.h"

#include <sst/core/rng/marsaglia.h>

namespace SST::IMPL::PortModule {

class RandomDrop : public SST::PortModule
{
public:
    SST_ELI_REGISTER_PORTMODULE(
        RandomDrop,
        "sst",
        "portmodules.random_drop",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Port module that will randomly drop events based on specified propability"
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "drop_prob", "Probability to drop event", "0.01" },
        { "drop_on_send",  "Controls whether to drop packetes during the send versus the default of on the receive", "false" },
        { "rngseed", "Set a seed for the random number generator used to control drops", "7" },
        { "verbose", "Debugging output", "false"}
    )

    RandomDrop(Params& params);

    // For serialization only
    RandomDrop() = default;
    ~RandomDrop();

    uintptr_t registerLinkAttachTool(const AttachPointMetaData& mdata) override;
    void      eventSent(uintptr_t key, Event*& ev) override;

    uintptr_t registerHandlerIntercept(const AttachPointMetaData& mdata) override;
    void      interceptHandler(uintptr_t key, Event*& data, bool& cancel) override;

    /**
       Called to determine if the PortModule should be installed on
       receives

       @return true if PortModule should be installed on receives
     */
    bool installOnReceive() override { return !drop_on_send_; }

    /**
       Called to determine if the PortModule should be installed on
       sends. NOTE: Install PortModules on sends will have a
       noticeable impact on performance, consider architecting things
       so that you can intercept on receives.

       @return true if PortModule should be installed on sends
     */
    bool installOnSend() override { return drop_on_send_; }


protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::IMPL::PortModule::RandomDrop);

private:
    double                 drop_prob_    = 0.01;
    bool                   verbose_      = false;
    bool                   drop_on_send_ = false;
    SST::RNG::MarsagliaRNG rng_;

    // String will store information about component and port names if
    // verbose is turned on.
    std::string* print_info_ = nullptr;
};

} // namespace SST::IMPL::PortModule

#endif // SST_CORE_IMPL_PORTMODULE_RANDOMDROP_H
