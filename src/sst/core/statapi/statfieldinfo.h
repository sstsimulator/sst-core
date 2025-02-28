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

#ifndef SST_CORE_STATAPI_STATFIELDINFO_H
#define SST_CORE_STATAPI_STATFIELDINFO_H

#include "sst/core/sst_types.h"

#include <map>
#include <string>

namespace SST::Statistics {

using fieldType_t = uint32_t;

/**
    \class StatisticFieldInfo

    The class for representing Statistic Output Fields
*/

class StatisticFieldTypeBase
{
public:
    virtual const char* fieldName() const = 0;
    virtual const char* shortName() const = 0;

    virtual ~StatisticFieldTypeBase() {}

    static StatisticFieldTypeBase* getField(fieldType_t field_type);

    // This is not a quick lookup; intended for checkpoint/restart only
    static fieldType_t getField(const char* field_short_name);

    static void checkRegisterConflict(const char* old_name, const char* new_name);

    static fieldType_t allocateFieldEnum();

protected:
    static void addField(fieldType_t id, StatisticFieldTypeBase* base);

private:
    static std::map<fieldType_t, StatisticFieldTypeBase*>* fields_;
    static fieldType_t                                     enum_counter_;
};

template <class T>
class StatisticFieldType : public StatisticFieldTypeBase
{
public:
    // constructor for initializing data
    StatisticFieldType(const char* name, const char* short_name)
    {
        checkRegisterConflict(field_name_, name);
        checkRegisterConflict(short_name_, short_name);
        field_name_ = name;
        short_name_ = short_name;
        if ( field_enum_ == 0 ) { field_enum_ = allocateFieldEnum(); }

        field_name_ = name;
        short_name_ = short_name;
        addField(field_enum_, this);
    }

    static const char* getFieldName() { return field_name_; }

    static const char* getShortName() { return short_name_; }

    static fieldType_t id() { return field_enum_; }

    const char* fieldName() const override { return getFieldName(); }

    const char* shortName() const override { return getShortName(); }

private:
    static Statistics::fieldType_t field_enum_;
    static const char*             field_name_;
    static const char*             short_name_;
};
template <class T>
fieldType_t StatisticFieldType<T>::field_enum_ = 0;
template <class T>
const char* StatisticFieldType<T>::field_name_ = nullptr;
template <class T>
const char* StatisticFieldType<T>::short_name_ = nullptr;

class StatisticFieldInfo
{
public:
    using fieldType_t   = ::SST::Statistics::fieldType_t;
    using fieldHandle_t = int32_t;

public:
    /** Construct a StatisticFieldInfo
     * @param statName - Name of the statistic registering this field.
     * @param fieldName - Name of the Field to be assigned.
     * @param fieldType - Data type of the field.
     */
    StatisticFieldInfo(const char* stat_name, const char* field_name, fieldType_t field_type);

    // Get Field Data
    /** Return the statistic name related to this field info */
    inline const std::string& getStatName() const { return stat_name_; }
    /** Return the field name related to this field info */
    inline const std::string& getFieldName() const { return field_name_; }
    /** Return the field type related to this field info */
    fieldType_t               getFieldType() const { return field_type_; }
    /** Return the field type related to this field info */
    std::string               getFieldUniqueName() const;

    /** Compare two field info structures
     * @param FieldInfo1 - a FieldInfo to compare against.
     * @return True if the Field Info structures are the same.
     */
    bool operator==(StatisticFieldInfo& field_info_1);

    /** Set the field handle
     * @param handle - The assigned field handle for this FieldInfo
     */
    void setFieldHandle(fieldHandle_t handle) { field_handle_ = handle; }

    /** Get the field handle
     * @return The assigned field handle.
     */
    fieldHandle_t getFieldHandle() { return field_handle_; }

    static const char* getFieldTypeShortName(fieldType_t type)
    {
        return StatisticFieldTypeBase::getField(type)->shortName();
    }

    static const char* getFieldTypeFullName(fieldType_t type)
    {
        return StatisticFieldTypeBase::getField(type)->fieldName();
    }

    template <typename T>
    static fieldType_t getFieldTypeFromTemplate()
    {
        return StatisticFieldType<T>::id();
    }

protected:
    StatisticFieldInfo() {} // For serialization only

private:
    std::string   stat_name_;
    std::string   field_name_;
    fieldType_t   field_type_;
    fieldHandle_t field_handle_;
};

} // namespace SST::Statistics

#endif // SST_CORE_STATAPI_STATFIELDINFO_H
