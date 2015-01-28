// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"

#include "sst/core/output.h"
#include <sst/core/statapi/statoutput.h>

//#include <map>

namespace SST {
namespace Statistics {

StatisticFieldInfo::StatisticFieldInfo(const char* statName, const char* fieldName, fieldType_t fieldType)
{
    m_statName  = statName; 
    m_fieldName = fieldName; 
    m_fieldType = fieldType;
    m_fieldHandle = -1;
}
    
bool StatisticFieldInfo::operator==(StatisticFieldInfo& FieldInfo1) 
{
    return ( /*(getStatName()  == FieldInfo1.getStatName())  &&*/
             (getFieldName() == FieldInfo1.getFieldName()) &&
             (getFieldType() == FieldInfo1.getFieldType()) );
}
    
////////////////////////////////////////////////////////////////////////////////    
    
StatisticOutput::StatisticOutput(Params& outputParameters)
 : Module()
{
    m_statOutputName = "StatisticOutput";
    m_outputParameters = outputParameters;
    m_highestFieldHandle = 0;
    m_currentFieldCompName = "";
    m_currentFieldStatName = "";
    m_statLoadLevel = 0;
}

StatisticOutput::~StatisticOutput()
{
}

// Start / Stop of register
void StatisticOutput::startRegisterFields(const char* componentName, const char* statisticName)
{
    m_currentFieldCompName = componentName;
    m_currentFieldStatName = statisticName;
}

void StatisticOutput::stopRegisterFields()
{
    m_currentFieldCompName = "";
    m_currentFieldStatName = "";
}

StatisticFieldInfo* StatisticOutput::addFieldToLists(const char* fieldName, fieldType_t fieldType)
{
    StatisticFieldInfo* NewStatFieldInfo;
    StatisticFieldInfo* ExistingStatFieldInfo;

    // Create a new Instance of a StatisticFieldInfo
    NewStatFieldInfo = new StatisticFieldInfo(m_currentFieldStatName.c_str(), fieldName, fieldType);

    // Now search the FieldInfoArray of type for a matching entry
    for (FieldInfoArray_t::iterator it_v = m_outputFieldInfoArray.begin(); it_v != m_outputFieldInfoArray.end(); it_v++) {
        ExistingStatFieldInfo = *it_v;
        
        // Check to see if the the New Stat Field matches any exisiting Stat Fields
        if (*ExistingStatFieldInfo == *NewStatFieldInfo) {
            // This field for this statistic has already been registered, so this cannot be registered
            return ExistingStatFieldInfo;
        }
    }
    
    // If we get here, then the New StatFieldInfo does not exist so add it
    m_outputFieldInfoArray.push_back(NewStatFieldInfo);
    
    return NewStatFieldInfo;
}

StatisticOutput::fieldHandle_t StatisticOutput::generateFileHandle(StatisticFieldInfo* FieldInfo)
{
    // Check to see if this field info has a handle assigned
    if (-1 == FieldInfo->getFieldHandle()) {
        // Not assigned, so assign it and increment the handle
        FieldInfo->setFieldHandle(m_highestFieldHandle);
        m_highestFieldHandle++;
    }
    return FieldInfo->getFieldHandle();
}

//// Routines to Manipulate Fields
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

void StatisticOutput::startOutputEntries(const char* componentName, const char* statisticName)
{
    m_currentFieldCompName = componentName;
    m_currentFieldStatName = statisticName;
    // Call the Derived class method
    implStartOutputEntries(componentName, statisticName);
}

void StatisticOutput::stopOutputEntries()
{
    m_currentFieldCompName = "";
    m_currentFieldStatName = "";
    // Call the Derived class method
    implStopOutputEntries();
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, int32_t data)  
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, uint32_t data)  
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, int64_t data)  
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, uint64_t data)  
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, float data)  
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, double data)
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

void StatisticOutput::outputField(fieldHandle_t fieldHandle, const char* data)
{
    // Call the Derived class method
    implOutputField(fieldHandle, data);
}

const char* StatisticOutput::getFieldTypeShortName(fieldType_t type)
{
    switch (type) {
        case StatisticFieldInfo::INT32  : return "i32"; break;
        case StatisticFieldInfo::UINT32 : return "u32"; break;
        case StatisticFieldInfo::INT64  : return "i64"; break;
        case StatisticFieldInfo::UINT64 : return "u64"; break;
        case StatisticFieldInfo::FLOAT  : return "f32"; break;
        case StatisticFieldInfo::DOUBLE : return "f64"; break;
        case StatisticFieldInfo::STR    : return "s"; break;
        default: return "INVALID"; break;    
    }
}


} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Statistics::StatisticFieldInfo);
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Statistics::StatisticOutput);


