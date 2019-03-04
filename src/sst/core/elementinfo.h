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

#ifndef SST_CORE_ELEMENTINFO_H
#define SST_CORE_ELEMENTINFO_H

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>
#include <sst/core/params.h>
#include <sst/core/statapi/statfieldinfo.h>

#include <sst/core/paramsInfo.h>
#include <sst/core/statsInfo.h>
#include <sst/core/defaultInfo.h>
#include <sst/core/portsInfo.h>
#include <sst/core/subcompSlotInfo.h>
#include <sst/core/interfaceInfo.h>
#include <sst/core/categoryInfo.h>
#include <sst/core/elementbuilder.h>

#include <string>
#include <vector>

#include <sst/core/elibase.h>

namespace SST {
class Component;
class Module;
class SubComponent;
class BaseComponent;
namespace Partition {
    class SSTPartitioner;
}
namespace Statistics {
  template <class T> class Statistic;
  class StatisticBase;
}
class RankInfo;
class SSTElementPythonModule;

/****************************************************
   Base classes for templated documentation classes
*****************************************************/

namespace ELI {

template <class T>
class DataBase {
 public:
  static T* get(const std::string& elemlib, const std::string& elem){
    if (!infos_) return nullptr;

    auto libiter = infos_->find(elemlib);
    if (libiter != infos_->end()){
      auto& submap = libiter->second;
      auto elemiter = submap.find(elem);
      if (elemiter != submap.end()){
        return elemiter->second;
      }
    }
    return nullptr;
  }

  static void add(const std::string& elemlib, const std::string& elem, T* info){
    if (!infos_){
      infos_ = std::unique_ptr<std::map<std::string, std::map<std::string, T*>>>(
                  new std::map<std::string, std::map<std::string, T*>>);
    }

    (*infos_)[elemlib][elem] = info;
  }

 private:
  static std::unique_ptr<std::map<std::string, std::map<std::string, T*>>> infos_;
};
template <class T> std::unique_ptr<std::map<std::string,std::map<std::string,T*>>> DataBase<T>::infos_;


template <class Policy, class... Policies>
class BuilderInfoImpl : public Policy, public BuilderInfoImpl<Policies...>
{
  using Parent=BuilderInfoImpl<Policies...>;
 public:
  template <class... Args> BuilderInfoImpl(const std::string& elemlib,
                                           const std::string& elem,
                                           Args&&... args)
    : Policy(args...), Parent(elemlib, elem, args...) //forward as l-values
  {
    DataBase<Policy>::add(elemlib,elem,this);
  }

  template <class XMLNode> void outputXML(XMLNode* node){
    Policy::outputXML(node);
    Parent::outputXML(node);
  }

  void toString(std::ostream& os) const {
    Parent::toString(os);
    Policy::toString(os);
  }
};

template <> class BuilderInfoImpl<void> {
 protected:
  template <class... Args> BuilderInfoImpl(Args&&... UNUSED(args))
  {
  }

  template <class XMLNode> void outputXML(XMLNode* UNUSED(node)){}

  void toString(std::ostream& UNUSED(os)) const {}

};

template <class Base, class T>
struct InstantiateBuilderInfo {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

class LoadedLibraries {
 public:
  static void addLoaded(const std::string& name){
    if (!loaded_){
      loaded_ = std::unique_ptr<std::set<std::string>>(new std::set<std::string>);
    }
    loaded_->insert(name);
  }

  static bool isLoaded(const std::string& name){
    if (loaded_){
      return loaded_->find(name) != loaded_->end();
    } else {
      return false; //nothing loaded yet
    }
  }

 private:
  static std::unique_ptr<std::set<std::string>> loaded_;
};

template <class Base> class InfoLibrary
{
 public:
  using BaseInfo = typename Base::BuilderInfo;

  BaseInfo* getInfo(const std::string& name) {
    auto iter = infos_.find(name);
    if (iter == infos_.end()){
      return nullptr;
    } else {
      return iter->second;
    }
  }

  int numEntries() const {
    return infos_.size();
  }

  const std::map<std::string, BaseInfo*> getMap() const {
    return infos_;
  }

  bool addInfo(const std::string& name, BaseInfo* info){
    infos_[name] = info;
    return true;
  }

 private:
  std::map<std::string, BaseInfo*> infos_;
};


template <class Base>
class InfoLibraryDatabase {
 public:
  using Library=InfoLibrary<Base>;
  using BaseInfo=typename Library::BaseInfo;

  static Library* getLibrary(const std::string& name){
    if (!libraries){
      libraries = std::make_unique<std::map<std::string,Library*>>();
    }
    auto iter = libraries->find(name);
    if (iter == libraries->end()){
      LoadedLibraries::addLoaded(name);
      auto* info = new Library;
      (*libraries)[name] = info;
      return info;
    } else {
      return iter->second;
    }
  }

 private:
  // Database - needs to be a pointer for static init order
  static std::unique_ptr<std::map<std::string,Library*>> libraries;
};

template <class Base>
 std::unique_ptr<std::map<std::string,InfoLibrary<Base>*>>
  InfoLibraryDatabase<Base>::libraries;

template <class Base>
struct ElementsInfo
{
  static InfoLibrary<Base>* getLibrary(const std::string& name){
    return InfoLibraryDatabase<Base>::getLibrary(name);
  }

  template <class T> static bool add(){
    return Base::template addDerivedInfo<T>(T::ELI_getLibrary(), T::ELI_getName());
  }
};
template <class Base, class T>
 const bool InstantiateBuilderInfo<Base,T>::loaded = ElementsInfo<Base>::template add<T>();


struct InfoDatabase {
  template <class T>
  static InfoLibrary<T>* getLibrary(const std::string& name){
    return InfoLibraryDatabase<T>::getLibrary(name);
  }
};

} //namespace ELI


/**************************************************************************
  Class and constexpr functions to extract integers from version number.
**************************************************************************/

struct SST_ELI_element_version_extraction {
    const unsigned major;
    const unsigned minor;
    const unsigned tertiary;

    constexpr unsigned getMajor() { return major; }
    constexpr unsigned getMinor() { return minor; }
    constexpr unsigned getTertiary() { return tertiary; }
};

constexpr unsigned SST_ELI_getMajorNumberFromVersion(SST_ELI_element_version_extraction ver) {
    return ver.getMajor();
}

constexpr unsigned SST_ELI_getMinorNumberFromVersion(SST_ELI_element_version_extraction ver) {
    return ver.getMinor();
}

constexpr unsigned SST_ELI_getTertiaryNumberFromVersion(SST_ELI_element_version_extraction ver) {
    return ver.getTertiary();
}


/**************************************************************************
  Macros used by elements to add element documentation
**************************************************************************/

#define SST_ELI_DECLARE_BASE(Base) \
  using __LocalEliBase = Base; \
  using InfoLibrary = ::SST::ELI::InfoLibrary<Base>; \
  static const char* ELI_baseName(){ return #Base; }

#define SST_ELI_DECLARE_INFO_COMMON() \
  template <class __TT> static bool addDerivedInfo(const std::string& lib, const std::string& elem){ \
    return addInfo(lib,elem,new BuilderInfo(lib,elem,(__TT*)nullptr)); \
  }

#define SST_ELI_DECLARE_INFO(...) \
  using BuilderInfo = ::SST::ELI::BuilderInfoImpl<__VA_ARGS__,SST::ELI::ProvidesDefaultInfo,void>; \
  SST_ELI_DECLARE_INFO_COMMON() \
  template <class... __Args> static bool addInfo(const std::string& elemlib, const std::string& elem, \
                                   ::SST::ELI::BuilderInfoImpl<__Args...>* info){ \
    return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem,info); \
  }

#define SST_ELI_DECLARE_DEFAULT_INFO() \
  using BuilderInfo = ::SST::ELI::BuilderInfoImpl<SST::ELI::ProvidesDefaultInfo,void>; \
  SST_ELI_DECLARE_INFO_COMMON() \
  template <class... __Args> static bool addInfo(const std::string& elemlib, const std::string& elem, \
                                   ::SST::ELI::BuilderInfoImpl<__Args...>* info){ \
    return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem,info); \
  }

#define SST_ELI_DECLARE_INFO_EXTERN(...) \
  using BuilderInfo = ::SST::ELI::BuilderInfoImpl<SST::ELI::ProvidesDefaultInfo,__VA_ARGS__,void>; \
  static bool addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info); \
  SST_ELI_DECLARE_INFO_COMMON()

#define SST_ELI_DECLARE_DEFAULT_INFO_EXTERN() \
  using BuilderInfo = ::SST::ELI::BuilderInfoImpl<SST::ELI::ProvidesDefaultInfo,void>; \
  SST_ELI_DECLARE_INFO_COMMON() \
  static bool addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info); 

#define SST_ELI_DEFINE_INFO_EXTERN(base) \
  bool base::addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info){ \
    return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem,info); \
  }

#define SST_ELI_EXTERN_DERIVED(base,cls,lib,name,version,desc) \
  bool ELI_isLoaded(); \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_DERIVED(base,cls,lib,name,version,desc) \
  bool ELI_isLoaded() { \
    return SST::ELI::InstantiateBuilder<base,cls>::isLoaded() \
      && SST::ELI::InstantiateBuilderInfo<base,cls>::isLoaded(); \
  } \
  SST_ELI_DEFAULT_INFO(lib,name,ELI_FORWARD_AS_ONE(version),desc)

#define SST_ELI_REGISTER_EXTERN(base,cls) \
  bool cls::ELI_isLoaded(){ \
    return SST::ELI::InstantiateBuilder<base,cls>::isLoaded() \
      && SST::ELI::InstantiateBuilderInfo<base,cls>::isLoaded(); \
  }

} //namespace SST

#endif // SST_CORE_ELEMENTINFO_H
