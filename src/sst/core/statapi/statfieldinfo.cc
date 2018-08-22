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

std::map<FieldId_t,StatisticFieldTypeBase*>* StatisticFieldTypeBase::fields_ = nullptr;

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
StatisticFieldTypeBase::getField(FieldId_t id)
{
  auto iter = fields_->find(id);
  if (iter == fields_->end()){
    Simulation::getSimulationOutput().fatal(CALL_INFO,1,
       "Invalid Field ID: %d", int(id));
  }
  return iter->second;
}

SST_REGISTER_STATISTIC_FIELD(int32_t, i32);
SST_REGISTER_STATISTIC_FIELD(uint32_t, u32);
SST_REGISTER_STATISTIC_FIELD(int64_t, i64);
SST_REGISTER_STATISTIC_FIELD(uint64_t, u64);
SST_REGISTER_STATISTIC_FIELD(float, f);
SST_REGISTER_STATISTIC_FIELD(double, d);

} //namespace Statistics
} //namespace SST
