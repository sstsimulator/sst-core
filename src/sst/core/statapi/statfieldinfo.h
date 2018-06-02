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

#ifndef _H_SST_CORE_STATISTICS_FIELDINFO
#define _H_SST_CORE_STATISTICS_FIELDINFO

#include "sst/core/sst_types.h"

namespace SST {
namespace Statistics {

////////////////////////////////////////////////////////////////////////////////
// Quick Support structures for checking type (Can's use std::is_same as it is only available in C++11)
template <typename, typename> struct is_type_same { static const bool value = false;};
template <typename T> struct is_type_same<T,T> { static const bool value = true;};    

////////////////////////////////////////////////////////////////////////////////
    
/**
    \class StatisticFieldInfo

	The class for representing Statistic Output Fields  
*/
class StatisticFieldInfo
{
public:
    /** Supported Field Types */
    enum fieldType_t {UNDEFINED, UINT32, UINT64, INT32, INT64, FLOAT, DOUBLE};
    typedef int32_t fieldHandle_t;

public:
    /** Construct a StatisticFieldInfo
     * @param statName - Name of the statistic registering this field.
     * @param fieldName - Name of the Field to be assigned.
     * @param fieldType - Data type of the field.
     */
    StatisticFieldInfo(const char* statName, const char* fieldName, fieldType_t fieldType);
   
    // Get Field Data
    /** Return the statistic name related to this field info */
    inline const std::string& getStatName() const {return m_statName;}
    /** Return the field name related to this field info */
    inline const std::string& getFieldName() const {return m_fieldName;}
    /** Return the field type related to this field info */
    fieldType_t getFieldType() const {return m_fieldType;}
    /** Return the field type related to this field info */
    std::string getFieldUniqueName() const;
    
    /** Compare two field info structures 
     * @param FieldInfo1 - a FieldInfo to compare against.
     * @return True if the Field Info structures are the same.
     */
    bool operator==(StatisticFieldInfo& FieldInfo1); 
    
    /** Set the field handle 
     * @param handle - The assigned field handle for this FieldInfo
     */
    void setFieldHandle(fieldHandle_t handle) {m_fieldHandle = handle;}

    /** Get the field handle 
     * @return The assigned field handle.
     */
    fieldHandle_t getFieldHandle() {return m_fieldHandle;}
    
    static const char* getFieldTypeShortName(fieldType_t type);
    static const char* getFieldTypeFullName(fieldType_t type);

    template<typename T>
    static fieldType_t getFieldTypeFromTemplate()
    {
        if (is_type_same<T, int32_t    >::value){return INT32; }
        if (is_type_same<T, uint32_t   >::value){return UINT32;}
        if (is_type_same<T, int64_t    >::value){return INT64; }
        if (is_type_same<T, uint64_t   >::value){return UINT64;}
        if (is_type_same<T, float      >::value){return FLOAT; }
        if (is_type_same<T, double     >::value){return DOUBLE;}
        
        // If we get here, the data type is undefined
        return UNDEFINED;
    }
    
protected:
    StatisticFieldInfo(){}; // For serialization only
    
private:   
    std::string   m_statName; 
    std::string   m_fieldName; 
    fieldType_t   m_fieldType;
    fieldHandle_t m_fieldHandle;

};
    
} //namespace Statistics
} //namespace SST

#endif
