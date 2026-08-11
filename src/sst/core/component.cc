// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/component.h"

#include "sst/core/exit.h"
#include "sst/core/factory.h"
#include "sst/core/simulation.h"

using namespace SST::Statistics;

namespace SST {

SST_ELI_DEFINE_INFO_EXTERN(Component)
SST_ELI_DEFINE_CTOR_EXTERN(Component)

Component::Component(ComponentId_t id) :
    BaseComponent(id)
{
    // my_info = sim->getComponentInfo(id);
    // currentlyLoadingSubComponent = my_info;
}

Clock::Group*
Component::getClockGroup(TimeConverter tc)
{
    auto it = clock_groups_.find(tc.getFactor());
    if ( it != clock_groups_.end() ) return it->second;

    // Need to create a clock group and register it with the Clock object
    Clock::Group* group = new Clock::Group();
    sim_->registerClockGroup(tc, group);
    clock_groups_[tc.getFactor()] = group;
    return group;
}


void
Component::serialize_order(SST::Core::Serialization::serializer& ser)
{
    BaseComponent::serialize_order(ser);
}

void
Component::serialize_final(SST::Core::Serialization::serializer& ser)
{
    // Need to serialize the ordered clock handlers
    SST_SER(clock_groups_);

    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        // Need to reregister the groups with their clocks
        for ( auto& [period, group] : clock_groups_ ) {
            sim_->registerClockGroup(period, group);
            group->activateOnRestart();
        }
    }
}


} // namespace SST
