// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/output.h"
#include <sst/core/statapi/statoutput.h>
#include <sst/core/statapi/statgroup.h>
#include <sst/core/stringize.h>
#include <sst/core/simulation.h>

namespace SST {
namespace Statistics {

#if 0
class NewStatOutput : public StatisticOutput
{
 public:
  SST_ELI_DECLARE_NEW_BASE(StatisticOutput,NewStatOutput) 
  SST_ELI_NEW_BASE_CTOR(int,int) 
};

class MyOutputTest : public NewStatOutput
{
 public:
  SST_ELI_REGISTER_DERIVED(
    NewStatOutput,
    MyOutputTest,
    "sst",
    "test",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Make sure eli works"
 )
    MyOutputTest(Params& outputParameters){}

    MyOutputTest(int a, int b){}

    bool checkOutputParameters() override {}

    /** Print out usage for this Statistic Output */
    void printUsage() override {}

    void implStartRegisterFields(StatisticBase *stat) override {}
    void implRegisteredField(fieldHandle_t fieldHandle) override {}
    void implStopRegisterFields() override {}

    void implStartRegisterGroup(StatisticGroup* group ) override {}
    void implStopRegisterGroup() override {}

    /** Indicate to Statistic Output that simulation started.
     *  Statistic output may perform any startup code here as necessary.
     */
    void startOfSimulation() override {}

    /** Indicate to Statistic Output that simulation ended.
     *  Statistic output may perform any shutdown code here as necessary.
     */
    void endOfSimulation() override {}

    /** Implementation function for the start of output.
     * This will be called by the Statistic Processing Engine to indicate that
     * a Statistic is about to send data to the Statistic Output for processing.
     * @param statistic - Pointer to the statistic object than the output can
     * retrieve data from.
     */
    void implStartOutputEntries(StatisticBase* statistic) override {}

    /** Implementation function for the end of output.
     * This will be called by the Statistic Processing Engine to indicate that
     * a Statistic is finished sending data to the Statistic Output for processing.
     * The Statistic Output can perform any output related functions here.
     */
    void implStopOutputEntries() override {}

    void implStartOutputGroup(StatisticGroup* group) override {}
    void implStopOutputGroup() override {}

    /** Implementation functions for output.
     * These will be called by the statistic to provide Statistic defined
     * data to be output.
     * @param fieldHandle - The handle to the registered statistic field.
     * @param data - The data related to the registered field to be output.
     */
    void outputField(fieldHandle_t fieldHandle, int32_t data) override {}
    void outputField(fieldHandle_t fieldHandle, uint32_t data) override {}
    void outputField(fieldHandle_t fieldHandle, int64_t data) override {}
    void outputField(fieldHandle_t fieldHandle, uint64_t data) override {}
    void outputField(fieldHandle_t fieldHandle, float data) override {}
    void outputField(fieldHandle_t fieldHandle, double data) override {}


};
#endif


////////////////////////////////////////////////////////////////////////////////    
    
StatisticOutput::StatisticOutput(Params& outputParameters)
 : Module()
{
    m_statOutputName = "StatisticOutput";
    m_outputParameters = outputParameters;
    m_highestFieldHandle = 0;
    m_currentFieldStatName = "";
}

SST_ELI_DEFINE_CTOR_EXTERN(StatisticOutput)
SST_ELI_DEFINE_INFO_EXTERN(StatisticOutput)

StatisticOutput::~StatisticOutput()
{
}

void StatisticOutput::registerStatistic(StatisticBase *stat)
{
    startRegisterFields(stat);
    stat->registerOutputFields(this);
    stopRegisterFields();
}

void StatisticOutput::registerGroup(StatisticGroup *group)
{
    implStartRegisterGroup(group);
    for ( auto &stat : group->stats ) {
        registerStatistic(stat);
    }
    implStopRegisterGroup();
}

// Start / Stop of register
void StatisticOutput::startRegisterFields(StatisticBase *statistic)
{
    m_currentFieldStatName = statistic->getStatName();
    implStartRegisterFields(statistic);
}

void StatisticOutput::stopRegisterFields()
{
    implStopRegisterFields();
    m_currentFieldStatName = "";
}

StatisticFieldInfo* StatisticOutput::addFieldToLists(const char* fieldName, fieldType_t fieldType)
{
    // Create a new (perhaps temporary) Instance of a StatisticFieldInfo
    StatisticFieldInfo* NewStatFieldInfo = new StatisticFieldInfo(m_currentFieldStatName.c_str(), fieldName, fieldType);
    auto iter = m_outputFieldNameMap.find(NewStatFieldInfo->getFieldUniqueName());
    if (iter != m_outputFieldNameMap.end()){
      delete NewStatFieldInfo;
      fieldHandle_t id = iter->second;
      StatisticFieldInfo* ExistingStatFieldInfo = m_outputFieldInfoArray[id];
      if (ExistingStatFieldInfo->getFieldType() != fieldType){
        Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
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

StatisticOutput::fieldHandle_t StatisticOutput::generateFieldHandle(StatisticFieldInfo* FieldInfo)
{
    // Check to see if this field info has a handle assigned
    if (-1 == FieldInfo->getFieldHandle()) {
        // Not assigned, so assign it and increment the handle
        FieldInfo->setFieldHandle(m_highestFieldHandle);
        m_highestFieldHandle++;
    }
    return FieldInfo->getFieldHandle();
}

//// Routines to Manipulate Fields (For Future Support)
//void StatisticOutput::setFieldHierarchy(fieldHandle_t fieldHandle, uint32_t Level, fieldHandle_t parent) 
//{
//}

StatisticFieldInfo* StatisticOutput::getRegisteredField(fieldHandle_t fieldHandle)
{
    if (fieldHandle <= m_highestFieldHandle) {
        return m_outputFieldInfoArray[fieldHandle];
    }
    return NULL;
}

void
StatisticOutput::outputField(fieldHandle_t UNUSED(fieldHandle), double UNUSED(data))
{
  Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
      "StatisticOutput %s does not support double output",
      getStatisticOutputName().c_str());
}

void
StatisticOutput::outputField(fieldHandle_t UNUSED(fieldHandle), float UNUSED(data))
{
  Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
      "StatisticOutput %s does not support float output",
      getStatisticOutputName().c_str());
}

void
StatisticOutput::outputField(fieldHandle_t UNUSED(fieldHandle), int32_t UNUSED(data))
{
  Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
      "StatisticOutput %s does not support int32_t output",
      getStatisticOutputName().c_str());
}

void
StatisticOutput::outputField(fieldHandle_t UNUSED(fieldHandle), uint32_t UNUSED(data))
{
  Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
      "StatisticOutput %s does not support uint32_t output",
      getStatisticOutputName().c_str());
}

void
StatisticOutput::outputField(fieldHandle_t UNUSED(fieldHandle), int64_t UNUSED(data))
{
  Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
      "StatisticOutput %s does not support int64_t output",
      getStatisticOutputName().c_str());
}

void
StatisticOutput::outputField(fieldHandle_t UNUSED(fieldHandle), uint64_t UNUSED(data))
{
  Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
      "StatisticOutput %s does not support uint64_t output",
      getStatisticOutputName().c_str());
}

void StatisticOutput::outputEntries(StatisticBase* statistic, bool endOfSimFlag)
{
    this->lock();
    startOutputEntries(statistic);
    statistic->outputStatisticData(this, endOfSimFlag);
    stopOutputEntries();
    this->unlock();
}

void StatisticOutput::startOutputEntries(StatisticBase* statistic)
{
    m_currentFieldStatName = statistic->getStatName();
    // Call the Derived class method
    implStartOutputEntries(statistic);
}

void StatisticOutput::stopOutputEntries()
{
    m_currentFieldStatName = "";
    // Call the Derived class method
    implStopOutputEntries();
}



void StatisticOutput::outputGroup(StatisticGroup* group, bool endOfSimFlag)
{
    this->lock();
    startOutputGroup(group);
    for ( auto & stat : group->stats ) {
        outputEntries(stat, endOfSimFlag);
    }
    stopOutputGroup();
    this->unlock();
}

void StatisticOutput::startOutputGroup(StatisticGroup* group)
{
    m_currentFieldStatName = group->name;
    // Call the Derived class method
    implStartOutputGroup(group);
}

void StatisticOutput::stopOutputGroup()
{
    m_currentFieldStatName = "";
    // Call the Derived class method
    implStopOutputGroup();
}

const char* StatisticOutput::getFieldTypeShortName(fieldType_t type)
{
    return StatisticFieldInfo::getFieldTypeShortName(type);
}

} //namespace Statistics
} //namespace SST
