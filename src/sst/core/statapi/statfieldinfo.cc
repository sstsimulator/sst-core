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

#include <sst/core/stringize.h>
#include <sst/core/simulation.h>
#include <sst/core/statapi/statfieldinfo.h>

namespace SST {
namespace Statistics {

std::map<fieldType_t,StatisticFieldTypeBase*>* StatisticFieldTypeBase::fields_ = nullptr;
fieldType_t StatisticFieldTypeBase::enumCounter_ = 0;

StatisticFieldInfo::StatisticFieldInfo(const char* statName, const char* fieldName, fieldType_t fieldType)
{
    m_statName  = statName; 
    m_fieldName = fieldName; 
    m_fieldType = fieldType;
    m_fieldHandle = -1;
}
    
bool StatisticFieldInfo::operator==(StatisticFieldInfo& FieldInfo1) 
{
    return ( (getFieldName() == FieldInfo1.getFieldName()) &&
             (getFieldType() == FieldInfo1.getFieldType()) );
}


std::string StatisticFieldInfo::getFieldUniqueName() const
{
  std::string strRtn;
  strRtn = getFieldName() + ".";
  strRtn += SST::to_string(getFieldType());
  return strRtn;
}

StatisticFieldTypeBase*
StatisticFieldTypeBase::getField(fieldType_t id)
{
  auto iter = fields_->find(id);
  if (iter == fields_->end()){
    Simulation::getSimulationOutput().fatal(CALL_INFO,1,
       "Invalid Field ID: %d", int(id));
  }
  return iter->second;
}

void
StatisticFieldTypeBase::addField(fieldType_t id, StatisticFieldTypeBase *base)
{
  if (!fields_){
    fields_ = new std::map<fieldType_t,StatisticFieldTypeBase*>;
  }
  (*fields_)[id] = base;
}

void
StatisticFieldTypeBase::checkRegisterConflict(const char *oldName, const char *newName)
{
  if (oldName && ::strcmp(oldName, newName)){
    Simulation::getSimulationOutput().fatal(CALL_INFO,1,
       "Conflicting names registered for field: %s != %s",
       oldName, newName);
  }
}

fieldType_t
StatisticFieldTypeBase::allocateFieldEnum()
{
  //increment first, start counting from zero
  enumCounter_++;
  return enumCounter_;
}

static StatisticFieldType<int32_t>  int32_register("int32_t", "i32");
static StatisticFieldType<int64_t>  int64_register("int64_t", "i64");
static StatisticFieldType<uint32_t> uint32_register("uint32_t", "u32");
static StatisticFieldType<uint64_t> uint64_register("uint64_t", "u64");
static StatisticFieldType<float>    float_register("float", "f32");
static StatisticFieldType<double>   double_register("double", "f64");


} //namespace Statistics
} //namespace SST
