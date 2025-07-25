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

#include "sst/core/statapi/statoutput.h"

#include "sst/core/output.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/statgroup.h"
#include "sst/core/stringize.h"
#include "sst/core/util/filesystem.h"


namespace SST::Statistics {

////////////////////////////////////////////////////////////////////////////////

StatisticOutput::StatisticOutput(Params& outputParameters)
{
    m_statOutputName   = "StatisticOutput";
    m_outputParameters = outputParameters;
}

SST_ELI_DEFINE_CTOR_EXTERN(StatisticOutput)
SST_ELI_DEFINE_INFO_EXTERN(StatisticOutput)

void
StatisticOutput::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(m_statOutputName);
    SST_SER(m_outputParameters);
}

void
StatisticOutput::outputGroup(StatisticGroup* group, bool endOfSimFlag)
{
    this->lock();
    startOutputGroup(group);
    for ( auto& stat : group->stats ) {
        output(stat, endOfSimFlag);
    }
    stopOutputGroup();
    this->unlock();
}

void
StatisticOutput::registerGroup(StatisticGroup* group)
{
    startRegisterGroup(group);
    for ( auto& stat : group->stats ) {
        registerStatistic(stat);
    }
    stopRegisterGroup();
}

void
StatisticOutput::printUsage()
{
    Simulation_impl::getSimulationOutput().output(
        "StatisticOutput does not provide usage message; use 'sst-info' instead");
}

Output&
StatisticOutput::getSimulationOutput()
{
    return Simulation_impl::getSimulationOutput();
}

RankInfo
StatisticOutput::getNumRanks()
{
    return Simulation_impl::getSimulation()->getNumRanks();
}

RankInfo
StatisticOutput::getRank()
{
    return Simulation_impl::getSimulation()->getRank();
}

SimTime_t
StatisticOutput::getCurrentSimCycle()
{
    return Simulation_impl::getSimulation()->getCurrentSimCycle();
}

std::string
StatisticOutput::getAbsolutePathForOutputFile(const std::string& filename)
{
    return Simulation_impl::filesystem.getAbsolutePath(filename);
}

StatisticFieldsOutput::StatisticFieldsOutput(Params& outputParameters) :
    StatisticOutput(outputParameters)
{
    m_highestFieldHandle   = 0;
    m_currentFieldStatName = "";
}

StatisticFieldInfo*
StatisticFieldsOutput::addFieldToLists(const char* fieldName, fieldType_t fieldType)
{
    // Create a new (perhaps temporary) Instance of a StatisticFieldInfo
    StatisticFieldInfo* NewStatFieldInfo = new StatisticFieldInfo(m_currentFieldStatName.c_str(), fieldName, fieldType);
    auto                iter             = m_outputFieldNameMap.find(NewStatFieldInfo->getFieldUniqueName());
    if ( iter != m_outputFieldNameMap.end() ) {
        delete NewStatFieldInfo;
        fieldHandle_t       id                    = iter->second;
        StatisticFieldInfo* ExistingStatFieldInfo = m_outputFieldInfoArray[id];
        if ( ExistingStatFieldInfo->getFieldType() != fieldType ) {
            Simulation_impl::getSimulationOutput().fatal(CALL_INFO, 1,
                "StatisticOutput %s registering the same column (%s) with two different types",
                getStatisticOutputName().c_str(), fieldName);
        }
        return ExistingStatFieldInfo;
    }

    // If we get here, then the New StatFieldInfo does not exist so add it to both
    // the Array and to the map
    m_outputFieldInfoArray.push_back(NewStatFieldInfo);
    m_outputFieldNameMap[NewStatFieldInfo->getFieldUniqueName()] = m_outputFieldInfoArray.size() - 1;

    return NewStatFieldInfo;
}

StatisticOutput::fieldHandle_t
StatisticFieldsOutput::generateFieldHandle(StatisticFieldInfo* FieldInfo)
{
    // Check to see if this field info has a handle assigned
    if ( -1 == FieldInfo->getFieldHandle() ) {
        // Not assigned, so assign it and increment the handle
        FieldInfo->setFieldHandle(m_highestFieldHandle);
        m_highestFieldHandle++;
    }
    return FieldInfo->getFieldHandle();
}

StatisticFieldInfo*
StatisticFieldsOutput::getRegisteredField(fieldHandle_t fieldHandle)
{
    if ( fieldHandle <= m_highestFieldHandle ) {
        return m_outputFieldInfoArray[fieldHandle];
    }
    return nullptr;
}

DISABLE_WARN_MISSING_NORETURN
void
StatisticFieldsOutput::outputField(fieldHandle_t UNUSED(fieldHandle), double UNUSED(data))
{
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "StatisticOutput %s does not support double output", getStatisticOutputName().c_str());
}

void
StatisticFieldsOutput::outputField(fieldHandle_t UNUSED(fieldHandle), float UNUSED(data))
{
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "StatisticOutput %s does not support float output", getStatisticOutputName().c_str());
}

void
StatisticFieldsOutput::outputField(fieldHandle_t UNUSED(fieldHandle), int32_t UNUSED(data))
{
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "StatisticOutput %s does not support int32_t output", getStatisticOutputName().c_str());
}

void
StatisticFieldsOutput::outputField(fieldHandle_t UNUSED(fieldHandle), uint32_t UNUSED(data))
{
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "StatisticOutput %s does not support uint32_t output", getStatisticOutputName().c_str());
}

void
StatisticFieldsOutput::outputField(fieldHandle_t UNUSED(fieldHandle), int64_t UNUSED(data))
{
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "StatisticOutput %s does not support int64_t output", getStatisticOutputName().c_str());
}

void
StatisticFieldsOutput::outputField(fieldHandle_t UNUSED(fieldHandle), uint64_t UNUSED(data))
{
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "StatisticOutput %s does not support uint64_t output", getStatisticOutputName().c_str());
}
REENABLE_WARNING

void
StatisticFieldsOutput::output(StatisticBase* statistic, bool endOfSimFlag)
{
    this->lock();
    startOutputEntries(statistic);
    statistic->outputStatisticFields(this, endOfSimFlag);
    stopOutputEntries();
    this->unlock();
}

void
StatisticFieldsOutput::startRegisterGroup(StatisticGroup* UNUSED(group))
{
    // do nothing by default
}

void
StatisticFieldsOutput::stopRegisterGroup()
{
    // do nothing by default
}

void
StatisticFieldsOutput::startOutputEntries(StatisticBase* statistic)
{
    m_currentFieldStatName = statistic->getStatName();
    // Call the Derived class method
    implStartOutputEntries(statistic);
}

void
StatisticFieldsOutput::stopOutputEntries()
{
    m_currentFieldStatName = "";
    // Call the Derived class method
    implStopOutputEntries();
}

void
StatisticFieldsOutput::startOutputGroup(StatisticGroup* group)
{
    m_currentFieldStatName = group->name;
}

void
StatisticFieldsOutput::stopOutputGroup()
{
    m_currentFieldStatName = "";
}

const char*
StatisticFieldsOutput::getFieldTypeShortName(fieldType_t type)
{
    return StatisticFieldInfo::getFieldTypeShortName(type);
}

void
StatisticFieldsOutput::registerStatistic(StatisticBase* stat)
{
    this->lock();
    startRegisterFields(stat);
    stat->registerOutputFields(this);
    stopRegisterFields();
    this->unlock();
}

// Start / Stop of register
void
StatisticFieldsOutput::startRegisterFields(StatisticBase* statistic)
{
    m_currentFieldStatName = statistic->getStatName();
}

void
StatisticFieldsOutput::stopRegisterFields()
{
    m_currentFieldStatName = "";
}

void
StatisticFieldsOutput::serialize_order(SST::Core::Serialization::serializer& ser)
{
    StatisticOutput::serialize_order(ser);
}

} // namespace SST::Statistics
