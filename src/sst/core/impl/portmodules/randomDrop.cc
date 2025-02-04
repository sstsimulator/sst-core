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

#include "sst_config.h"

#include "sst/core/impl/portmodules/randomDrop.h"

#include "sst/core/event.h"
#include "sst/core/params.h"

namespace SST {
namespace IMPL {
namespace PortModule {

RandomDrop::RandomDrop(Params& params) : rng_(7, 13)
{
    // Restart the RNG to ensure completely consistent results
    // Seed with user-provided seed
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng_.restart(z_seed, 13);

    drop_prob_    = params.find<double>("drop_prob", 0.01);
    verbose_      = params.find<bool>("verbose", false);
    drop_on_send_ = params.find<bool>("drop_on_send");
}

RandomDrop::~RandomDrop()
{
    if ( nullptr != print_info_ ) delete print_info_;
}

uintptr_t
RandomDrop::registerLinkAttachTool(const AttachPointMetaData& mdata)
{
    if ( verbose_ && nullptr == print_info_ ) {
        const EventHandlerMetaData& md = dynamic_cast<const EventHandlerMetaData&>(mdata);
        print_info_                    = new std::string;
        print_info_->append(md.comp_name).append("/").append(md.port_name);
    }
    return 0;
}

void
RandomDrop::eventSent(uintptr_t UNUSED(key), Event*& ev)
{
    double pull = rng_.nextUniform();

    if ( pull < drop_prob_ ) {
        if ( verbose_ ) {
            getSimulationOutput().output(
                "(%" PRI_SIMTIME ") %s: Dropping event on send\n", getCurrentSimCycle(), print_info_->c_str());
        }
        delete ev;
        ev = nullptr;
    }
}

uintptr_t
RandomDrop::registerHandlerIntercept(const AttachPointMetaData& mdata)
{
    if ( verbose_ && nullptr == print_info_ ) {
        const EventHandlerMetaData& md = dynamic_cast<const EventHandlerMetaData&>(mdata);
        print_info_                    = new std::string;
        print_info_->append(md.comp_name).append("/").append(md.port_name);
    }
    return 0;
}

void
RandomDrop::interceptHandler(uintptr_t UNUSED(key), Event*& data, bool& cancel)
{
    double pull = rng_.nextUniform();

    if ( pull < drop_prob_ ) {
        if ( verbose_ ) {
            getSimulationOutput().output(
                "(%" PRI_SIMTIME ") %s: Dropping event on receive\n", getCurrentSimCycle(), print_info_->c_str());
        }
        delete data;
        cancel = true;
    }
    else {
        cancel = false;
    }
}

void
RandomDrop::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::PortModule::serialize_order(ser);

    ser& drop_prob_;
    ser& verbose_;
    ser& drop_on_send_;
    ser& rng_;
    ser& print_info_;
}


} // namespace PortModule
} // namespace IMPL
} // namespace SST
