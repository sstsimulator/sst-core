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
#include "sst/core/configGraph.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/stringize.h"
#include "sst/core/warnmacros.h"

#include <cstdint>
#include <string>


namespace SST {

SST_ELI_DEFINE_INFO_EXTERN(PortModule)
SST_ELI_DEFINE_CTOR_EXTERN(PortModule)

PortModule::PortModule()
{
    ComponentId_t  comp;
    PortModuleId_t id;
    std::tie(comp, id) = BaseComponent::port_module_id_;
    // Differentiate restart and regular startup by whether comp is a valid ID
    if ( comp != UNSET_COMPONENT_ID ) {
        component_ = Simulation_impl::getSimulation()->getComponent(comp);
        id_        = id;
        name_      = component_->getName() + "." + id_.first + "." + std::to_string(id_.second);
    }
}

uintptr_t
PortModule::registerLinkAttachTool(const AttachPointMetaData& UNUSED(mdata))
{
    return 0;
}

void
PortModule::serializeEventAttachPointKey(SST::Core::Serialization::serializer& ser, uintptr_t& key)
{
    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        key = 0;
    }
}

uintptr_t
PortModule::registerHandlerIntercept(const AttachPointMetaData& UNUSED(mdata))
{
    return 0;
}

void
PortModule::serializeHandlerInterceptPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& key)
{
    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        key = 0;
    }
}

void
PortModule::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(component_);
    SST_SER(name_);
}

const std::string&
PortModule::getName() const
{
    return name_;
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
PortModule::getCurrentSimTime(TimeConverter& tc) const
{
    return component_->getCurrentSimTime(tc);
}

SimTime_t
PortModule::getCurrentSimTime(TimeConverter* tc) const
{
    return component_->getCurrentSimTime(*tc);
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

Statistics::StatisticProcessingEngine*
PortModule::getStatEngine() const
{
    return Simulation_impl::getSimulation()->getStatisticsProcessingEngine();
}

uint8_t
PortModule::getStatisticValidityAndLevel(const std::string& statistic_name) const
{
    return Factory::getFactory()->GetStatisticValidityAndEnableLevel(getEliType(), statistic_name);
}

std::pair<bool, Params>
PortModule::isStatisticEnabled(const std::string& statistic_name, const uint8_t min_level)
{
    Params                params;
    SST::ConfigPortModule config = component_->getPortModuleConfig(id_);
    auto                  stat   = config.per_stat_configs.find(statistic_name);
    if ( stat != config.per_stat_configs.end() ) {
        return std::make_pair(true, stat->second);
    }

    // If our level is 0, use stat engine's global level
    uint8_t enable_level = ((config.stat_load_level == STATISTICLOADLEVELUNINITIALIZED))
                               ? getStatEngine()->getStatLoadLevel()
                               : config.stat_load_level;
    if ( enable_level >= min_level ) {
        return std::make_pair(true, config.all_stat_config);
    }
    return std::make_pair(false, params);
}


void
PortModule::vfatal(
    uint32_t line, const char* file, const char* func, int exit_code, const char* format, va_list arg) const
{
    Output      abort("Rank: @R,@I, time: @t - called in file: @f, line: @l, function: @p", 5, -1, Output::STDOUT);
    std::string comp_name = component_ ? component_->getName() : "";
    std::string prologue  = format_string(
        "Element name: PortModule, type: %s (Associated component: %s)", getEliType().c_str(), comp_name.c_str());

    std::string msg = vformat_string(format, arg);
    abort.fatal(line, file, func, exit_code, "\n%s\n%s\n", prologue.c_str(), msg.c_str());
}

void
PortModule::fatal(uint32_t line, const char* file, const char* func, int exit_code, const char* format, ...) const
{
    va_list arg;
    va_start(arg, format);
    vfatal(line, file, func, exit_code, format, arg);
    va_end(arg);
}


} // namespace SST
