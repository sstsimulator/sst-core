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

#include "sst/core/subcomponent.h"

#include "sst/core/factory.h"

namespace SST {

SST_ELI_DEFINE_INFO_EXTERN(SubComponent)
SST_ELI_DEFINE_CTOR_EXTERN(SubComponent)

SubComponent::SubComponent(ComponentId_t id) :
    BaseComponent(id)
{}

void
SubComponent::serialize_order(SST::Core::Serialization::serializer& ser)
{
    BaseComponent::serialize_order(ser);
}

} // namespace SST
