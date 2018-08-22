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
#include <map>

namespace SST {
namespace Statistics {

using FieldId_t = uint32_t;

struct any_numeric_type {};
struct any_integer_type {};
struct any_floating_type {};

struct FieldEnum {};

template <class T, class IntegerType>
IntegerType allocateEnum() {
  static IntegerType counter = 0;
  auto ret = counter;
  ++counter;
  return ret;
}
    
/**
    \class StatisticFieldInfo

	The class for representing Statistic Output Fields  
*/

class StatisticFieldTypeBase {
 public:
  template <class T> static void registerField(FieldId_t id);

  virtual const char* fieldName() const = 0;
  virtual const char* shortName() const = 0;

  static StatisticFieldTypeBase* getField(FieldId_t id);

 private:
  static std::map<FieldId_t,StatisticFieldTypeBase*>* fields_;
};

template <class T>
class StatisticFieldType : public StatisticFieldTypeBase {
 public:
  static void registerName(const char* name, const char* shortName){
    fieldName_ = name;
    shortName_ = shortName;
  }

  static const char* getFieldName(){
    return fieldName_;
  }

  static const char* getShortName(){
    return shortName_;
  }

  static FieldId_t id() {
    return fieldEnum_;
  }

  const char* fieldName() const override {
    return getFieldName();
  }

  const char* shortName() const override {
    return getShortName();
  }

 private:
  static Statistics::FieldId_t fieldEnum_;
  static const char* fieldName_;
  static const char* shortName_;
};
template <class T> FieldId_t StatisticFieldType<T>::fieldEnum_ = allocateEnum<FieldEnum,FieldId_t>();
template <class T> const char* StatisticFieldType<T>::fieldName_ = nullptr;
template <class T> const char* StatisticFieldType<T>::shortName_ = nullptr;

template <class T> void
StatisticFieldTypeBase::registerField(FieldId_t id){
  if (!fields_){
    fields_ = new std::map<FieldId_t,StatisticFieldTypeBase*>;
  }
  auto iter = fields_->find(id);
  if (iter == fields_->end()){
    fields_[id] = new StatisticFieldType<T>;
  }
}

class StatisticFieldInfo
{
 public:
  using fieldType_t = FieldId_t;
  using fieldHandle_t = int32_t;

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
    
  static const char* getFieldTypeShortName(fieldType_t type){
    return StatisticFieldTypeBase::getField(type)->shortName();
  }

  static const char* getFieldTypeFullName(fieldType_t type){
    return StatisticFieldTypeBase::getField(type)->fieldName();
  }

  template<typename T>
  static FieldId_t getFieldTypeFromTemplate(){
    return StatisticFieldType<T>::id();
  }
    
 protected:
  StatisticFieldInfo(){} // For serialization only
    
 private:
  std::string   m_statName;
  std::string   m_fieldName;
  fieldType_t   m_fieldType;
  fieldHandle_t m_fieldHandle;

};

template <class FieldType>
struct StatisticFieldRegister {
  StatisticFieldRegister(const char* longName, const char* shortName){
    StatisticFieldType<FieldType>::registerName(longName, shortName);
  }
};

#define SST_REGISTER_STATISTIC_FIELD(fullName, shortName) \
  static StatisticFieldRegister<fullName> register_##shortName(#fullName, #shortName)
    
} //namespace Statistics
} //namespace SST

#endif
