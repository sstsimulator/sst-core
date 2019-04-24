// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
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

using fieldType_t = uint32_t;
    
/**
    \class StatisticFieldInfo

	The class for representing Statistic Output Fields  
*/

class StatisticFieldTypeBase {
 public:
  virtual const char* fieldName() const = 0;
  virtual const char* shortName() const = 0;

  virtual ~StatisticFieldTypeBase(){}

  static StatisticFieldTypeBase* getField(fieldType_t fieldType);

  static void checkRegisterConflict(const char* oldName, const char* newName);

  static fieldType_t allocateFieldEnum();

 protected:
  static void addField(fieldType_t id, StatisticFieldTypeBase* base);

 private:
  static std::map<fieldType_t,StatisticFieldTypeBase*>* fields_;
  static fieldType_t enumCounter_;
};

template <class T>
class StatisticFieldType : public StatisticFieldTypeBase {
 public:
  //constructor for initializing data
  StatisticFieldType(const char* name, const char* shortName){
    checkRegisterConflict(fieldName_, name);
    checkRegisterConflict(shortName_, shortName);
    fieldName_ = name;
    shortName_ = shortName;
    if (fieldEnum_ == 0){
      fieldEnum_ = allocateFieldEnum();
    }

    fieldName_ = name;
    shortName_ = shortName;
    addField(fieldEnum_, this);
  }

  static const char* getFieldName(){
    return fieldName_;
  }

  static const char* getShortName(){
    return shortName_;
  }

  static fieldType_t id() {
    return fieldEnum_;
  }

  const char* fieldName() const override {
    return getFieldName();
  }

  const char* shortName() const override {
    return getShortName();
  }

 private:
  static Statistics::fieldType_t fieldEnum_;
  static const char* fieldName_;
  static const char* shortName_;
};
template <class T> fieldType_t StatisticFieldType<T>::fieldEnum_ = 0;
template <class T> const char* StatisticFieldType<T>::fieldName_ = nullptr;
template <class T> const char* StatisticFieldType<T>::shortName_ = nullptr;


class StatisticFieldInfo
{
 public:
  using fieldType_t = ::SST::Statistics::fieldType_t;
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
  static fieldType_t getFieldTypeFromTemplate(){
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


    
} //namespace Statistics
} //namespace SST

#endif
