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
#include <sst/core/statapi/statfieldinfo.h>

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

const char* StatisticFieldInfo::getFieldTypeShortName(fieldType_t type)
{
    switch (type) {
        case StatisticFieldInfo::INT32  : return "i32"; break;
        case StatisticFieldInfo::UINT32 : return "u32"; break;
        case StatisticFieldInfo::INT64  : return "i64"; break;
        case StatisticFieldInfo::UINT64 : return "u64"; break;
        case StatisticFieldInfo::FLOAT  : return "f32"; break;
        case StatisticFieldInfo::DOUBLE : return "f64"; break;
        default: return "INVALID"; break;    
    }
}

const char* StatisticFieldInfo::getFieldTypeFullName(fieldType_t type)
{
    switch (type) {
        case StatisticFieldInfo::INT32  : return "INT32"; break;
        case StatisticFieldInfo::UINT32 : return "UINT32"; break;
        case StatisticFieldInfo::INT64  : return "INT64"; break;
        case StatisticFieldInfo::UINT64 : return "UINT64"; break;
        case StatisticFieldInfo::FLOAT  : return "FLOAT"; break;
        case StatisticFieldInfo::DOUBLE : return "DOUBLE"; break;
        default: return "INVALID"; break;    
    }
}

} //namespace Statistics
} //namespace SST
