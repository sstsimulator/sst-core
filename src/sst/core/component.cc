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

#include "sst/core/component.h"

#include "sst/core/exit.h"
#include "sst/core/factory.h"
#include "sst/core/simulation_impl.h"

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


void
Component::serialize_order(SST::Core::Serialization::serializer& ser)
{
    BaseComponent::serialize_order(ser);
}

} // namespace SST
