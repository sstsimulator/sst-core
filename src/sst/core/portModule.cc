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

#include "sst/core/portModule.h"

#include "sst/core/baseComponent.h"
#include "sst/core/warnmacros.h"

namespace SST {

SST_ELI_DEFINE_INFO_EXTERN(PortModule)
SST_ELI_DEFINE_CTOR_EXTERN(PortModule)

PortModule::PortModule() {}

uintptr_t
PortModule::registerLinkAttachTool(const AttachPointMetaData& UNUSED(mdata))
{
    return 0;
}

void
PortModule::serializeEventAttachPointKey(SST::Core::Serialization::serializer& ser, uintptr_t& key)
{
    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) { key = 0; }
}

uintptr_t
PortModule::registerHandlerIntercept(const AttachPointMetaData& UNUSED(mdata))
{
    return 0;
}

void
PortModule::serializeHandlerInterceptPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& key)
{
    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) { key = 0; }
}

void
PortModule::setComponent(SST::BaseComponent* comp)
{
    component_ = comp;
}


void
PortModule::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ser& component_;
}


UnitAlgebra
PortModule::getCoreTimeBase() const
{
    return component_->getCoreTimeBase();
}

SimTime_t
PortModule::getCurrentSimCycle() const
{
    return component_->getCurrentSimCycle();
}

int
PortModule::getCurrentPriority() const
{
    return component_->getCurrentPriority();
}

UnitAlgebra
PortModule::getElapsedSimTime() const
{
    return component_->getElapsedSimTime();
}

Output&
PortModule::getSimulationOutput() const
{
    return component_->getSimulationOutput();
}

SimTime_t
PortModule::getCurrentSimTime(TimeConverter* tc) const
{
    return component_->getCurrentSimTime(tc);
}

SimTime_t
PortModule::getCurrentSimTime(const std::string& base) const
{
    return component_->getCurrentSimTime(base);
}

SimTime_t
PortModule::getCurrentSimTimeNano() const
{
    return component_->getCurrentSimTimeNano();
}

SimTime_t
PortModule::getCurrentSimTimeMicro() const
{
    return component_->getCurrentSimTimeMicro();
}

SimTime_t
PortModule::getCurrentSimTimeMilli() const
{
    return component_->getCurrentSimTimeMilli();
}

} // namespace SST
