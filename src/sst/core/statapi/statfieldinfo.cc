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

#include "sst/core/statapi/statfieldinfo.h"

#include "sst/core/output.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/stringize.h"

namespace SST::Statistics {

std::map<fieldType_t, StatisticFieldTypeBase*>* StatisticFieldTypeBase::fields_       = nullptr;
fieldType_t                                     StatisticFieldTypeBase::enum_counter_ = 0;

StatisticFieldInfo::StatisticFieldInfo(const char* stat_name, const char* field_name, fieldType_t field_type)
{
    stat_name_    = stat_name;
    field_name_   = field_name;
    field_type_   = field_type;
    field_handle_ = -1;
}

bool
StatisticFieldInfo::operator==(StatisticFieldInfo& field_info_1)
{
    return ((getFieldName() == field_info_1.getFieldName()) && (getFieldType() == field_info_1.getFieldType()));
}

std::string
StatisticFieldInfo::getFieldUniqueName() const
{
    std::string str_rtn;
    str_rtn = getFieldName() + ".";
    str_rtn += std::to_string(getFieldType());
    return str_rtn;
}

StatisticFieldTypeBase*
StatisticFieldTypeBase::getField(fieldType_t id)
{
    auto iter = fields_->find(id);
    if ( iter == fields_->end() ) {
        Simulation_impl::getSimulationOutput().fatal(CALL_INFO, 1, "Invalid Field ID: %d", int(id));
    }
    return iter->second;
}

fieldType_t
StatisticFieldTypeBase::getField(const char* field_short_name)
{
    for ( auto iter = fields_->begin(); iter != fields_->end(); iter++ ) {
        if ( strcmp(iter->second->shortName(), field_short_name) ) { return iter->first; }
    }
    Simulation_impl::getSimulationOutput().fatal(
        CALL_INFO, 1, "Look up field name: %s; No such field found", field_short_name);
    return 0;
}

void
StatisticFieldTypeBase::addField(fieldType_t id, StatisticFieldTypeBase* base)
{
    if ( !fields_ ) { fields_ = new std::map<fieldType_t, StatisticFieldTypeBase*>; }
    (*fields_)[id] = base;
}

void
StatisticFieldTypeBase::checkRegisterConflict(const char* old_name, const char* new_name)
{
    if ( old_name && ::strcmp(old_name, new_name) ) {
        Simulation_impl::getSimulationOutput().fatal(
            CALL_INFO, 1, "Conflicting names registered for field: %s != %s", old_name, new_name);
    }
}

fieldType_t
StatisticFieldTypeBase::allocateFieldEnum()
{
    // increment first, start counting from zero
    enum_counter_++;
    return enum_counter_;
}

static StatisticFieldType<int32_t>  int32_register("int32_t", "i32");
static StatisticFieldType<int64_t>  int64_register("int64_t", "i64");
static StatisticFieldType<uint32_t> uint32_register("uint32_t", "u32");
static StatisticFieldType<uint64_t> uint64_register("uint64_t", "u64");
static StatisticFieldType<float>    float_register("float", "f32");
static StatisticFieldType<double>   double_register("double", "f64");

} // namespace SST::Statistics
