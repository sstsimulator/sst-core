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

#ifndef SST_CORE_FACTORY_INFO_H
#define SST_CORE_FACTORY_INFO_H

#include "sst/core/eli/elibase.h"
#include <type_traits>

namespace SST {
namespace ELI {

template <class Base, class... Args>
struct Builder
{
  typedef Base* (*createFxn)(Args...);

  virtual Base* create(Args... ctorArgs) = 0;

  template <class NewBase>
  using ChangeBase = Builder<NewBase,Args...>;
};

template <class Base, class... CtorArgs>
class BuilderLibrary
{
 public:
  using BaseBuilder = Builder<Base,CtorArgs...>;

  BuilderLibrary(const std::string& name) :
    name_(name)
  {
  }

  BaseBuilder* getBuilder(const std::string& name) {
    auto iter = factories_.find(name);
    if (iter == factories_.end()){
      return nullptr;
    } else {
      return iter->second;
    }
  }

  const std::map<std::string, BaseBuilder*>& getMap() const {
    return factories_;
  }

  void readdBuilder(const std::string& name, BaseBuilder* fact){
    factories_[name] = fact;
  }

  bool addBuilder(const std::string& elem, BaseBuilder* fact){
    readdBuilder(elem, fact);
    return addLoader(name_, elem, fact);
  }

  template <class NewBase>
  using ChangeBase = BuilderLibrary<NewBase,CtorArgs...>;

 private:
  bool addLoader(const std::string& elemlib, const std::string& elem, BaseBuilder* fact);

  std::map<std::string, BaseBuilder*> factories_;

  std::string name_;
};

template <class Base, class... CtorArgs>
class BuilderLibraryDatabase {
 public:
  using Library=BuilderLibrary<Base,CtorArgs...>;
  using BaseFactory=typename Library::BaseBuilder;
  using Map=std::map<std::string,Library*>;

  static Library* getLibrary(const std::string& name){
    if (!libraries){
      libraries = new std::map<std::string,Library*>;
    }
    auto iter = libraries->find(name);
    if (iter == libraries->end()){
      auto* info = new Library(name);
      (*libraries)[name] = info;
      return info;
    } else {
      return iter->second;
    }
  }

  template <class NewBase>
  using ChangeBase = BuilderLibraryDatabase<NewBase,CtorArgs...>;

 private:
  // Database - needs to be a pointer for static init order
  static Map* libraries;
};

template <class Base, class... CtorArgs> typename BuilderLibraryDatabase<Base,CtorArgs...>::Map*
  BuilderLibraryDatabase<Base,CtorArgs...>::libraries = nullptr;

template <class Base, class Builder, class... CtorArgs>
struct BuilderLoader : public LibraryLoader {
  BuilderLoader(const std::string& elemlib,
                const std::string& elem,
                Builder* builder) :
    elemlib_(elemlib), elem_(elem), builder_(builder)
  {
  }

  void load() override {
    BuilderLibraryDatabase<Base,CtorArgs...>::getLibrary(elemlib_)
        ->readdBuilder(elem_,builder_);
  }

 private:
  std::string elemlib_;
  std::string elem_;
  Builder* builder_;
};

template <class Base, class... CtorArgs>
bool BuilderLibrary<Base,CtorArgs...>::addLoader(const std::string& elemlib, const std::string& elem,
                                                 BaseBuilder *fact){
  auto loader = new BuilderLoader<Base,BaseBuilder,CtorArgs...>(elemlib, elem, fact);
  return ELI::LoadedLibraries::addLoader(elemlib, elem, loader);
}

template <class Base, class T>
struct InstantiateBuilder {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

template <class Base, class T> const bool InstantiateBuilder<Base,T>::loaded
  = Base::Ctor::template add<T>();

template <class Base, class T, class Enable=void>
struct Allocator
{
  template <class... Args>
  T* operator()(Args&&... args){
    return new T(std::forward<Args>(args)...);
  }
};

template <class Base, class T>
struct CachedAllocator
{
  template <class... Args>
  Base* operator()(Args&&... ctorArgs) {
    if (!cached_){
      cached_ = new T(std::forward<Args>(ctorArgs)...);
    }
    return cached_;
  }

  static Base* cached_;
};
template <class Base, class T>
  Base* CachedAllocator<Base,T>::cached_ = nullptr;

template <class T, class Base, class... Args>
struct DerivedBuilder : public Builder<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    return Allocator<Base,T>()(std::forward<Args>(ctorArgs)...);
  }
};


template <class T, class U>
struct is_tuple_constructible : public std::false_type {};

template <class T, class... Args>
struct is_tuple_constructible<T, std::tuple<Args...>> :
  public std::is_constructible<T, Args...>
{
};

struct BuilderDatabase {
  template <class T, class... Args>
  static BuilderLibrary<T,Args...>* getLibrary(const std::string& name){
    return BuilderLibraryDatabase<T,Args...>::getLibrary(name);
  }
};

template <class Base, class CtorTuple>
struct ElementsBuilder {};

template <class Base, class... Args>
struct ElementsBuilder<Base, std::tuple<Args...>>
{
  static BuilderLibrary<Base,Args...>* getLibrary(const std::string& name){
    return BuilderLibraryDatabase<Base,Args...>::getLibrary(name);
  }

  template <class T> static Builder<Base,Args...>* makeBuilder(){
    return new DerivedBuilder<T,Base,Args...>();
  }

};


/**
 @class ExtendedCtor
 Implements a constructor for a derived base as usually happens with subcomponents, e.g.
 class U extends API extends Subcomponent. You can construct U as either an API*
 or a Subcomponent* depending on usage.
*/
template <class NewCtor, class OldCtor>
struct ExtendedCtor
{
  template <class T>
  using is_constructible = typename NewCtor::template is_constructible<T>;

  /**
    The derived Ctor can "block" the more abstract Ctor, meaning an object
    should only be instantiated as the most derived type. enable_if here
    checks if both the derived API and the parent API are still valid
  */
  template <class T>
  static typename std::enable_if<OldCtor::template is_constructible<T>::value,bool>::type
  add(){
      //if abstract, force an allocation to generate meaningful errors
    return NewCtor::template add<T>() && OldCtor::template add<T>();
  }

  template <class T>
  static typename std::enable_if<!OldCtor::template is_constructible<T>::value,bool>::type
  add(){
      //if abstract, force an allocation to generate meaningful errors
    return NewCtor::template add<T>();
  }

  template <class __NewCtor>
  using ExtendCtor = ExtendedCtor<__NewCtor, ExtendedCtor<NewCtor,OldCtor>>;

  template <class NewBase>
  using ChangeBase = typename NewCtor::template ChangeBase<NewBase>;
};

template <class Base, class... Args>
struct SingleCtor
{
  template <class T>
  using is_constructible = std::is_constructible<T,Args...>;

  template <class T> static bool add(){
    //if abstract, force an allocation to generate meaningful errors
    auto* fact = new DerivedBuilder<T,Base,Args...>;
    return Base::addBuilder(T::ELI_getLibrary(),T::ELI_getName(),fact);
  }

  template <class NewBase>
  using ChangeBase = SingleCtor<NewBase, Args...>;

  template <class NewCtor>
  using ExtendCtor = ExtendedCtor<NewCtor, SingleCtor<Base,Args...>>;
};

template <class Base, class Ctor, class... Ctors>
struct CtorList : public CtorList<Base,Ctors...>
{
  template <class T>  //if T is constructible with Ctor arguments
  using is_constructible = typename std::conditional<is_tuple_constructible<T,Ctor>::value,
                 std::true_type, //yes, constructible
                 typename CtorList<Base,Ctors...>::template is_constructible<T> //not constructible here but maybe later
               >::type;

  template <class T, int NumValid=0, class U=T>
  static typename std::enable_if<std::is_abstract<U>::value || is_tuple_constructible<U,Ctor>::value, bool>::type
  add(){
    //if abstract, force an allocation to generate meaningful errors
    auto* fact = ElementsBuilder<Base,Ctor>::template makeBuilder<U>();
    Base::addBuilder(T::ELI_getLibrary(),T::ELI_getName(),fact);
    return CtorList<Base,Ctors...>::template add<T,NumValid+1>();
  }

  template <class T, int NumValid=0, class U=T>
  static typename std::enable_if<!std::is_abstract<U>::value && !is_tuple_constructible<U,Ctor>::value, bool>::type
  add(){
    return CtorList<Base,Ctors...>::template add<T,NumValid>();
  }

  template <class NewBase>
  using ChangeBase = CtorList<NewBase, Ctor, Ctors...>;

};

template <int NumValid>
struct NoValidConstructorsForDerivedType {
  static constexpr bool atLeastOneValidCtor = true;
};

template <> class NoValidConstructorsForDerivedType<0> {};

template <class Base> struct CtorList<Base,void>
{
  template <class T>
  using is_constructible = std::false_type;

  template <class T,int numValidCtors>
  static bool add(){
    return NoValidConstructorsForDerivedType<numValidCtors>::atLeastOneValidCtor;
  }
};

}
}

#define ELI_CTOR(...) std::tuple<__VA_ARGS__>
#define ELI_DEFAULT_CTOR() std::tuple<>

#define SST_ELI_CTORS_COMMON(...) \
  using Ctor = ::SST::ELI::CtorList<__LocalEliBase,__VA_ARGS__,void>; \
  template <class __TT, class... __CtorArgs> \
  using DerivedBuilder = ::SST::ELI::DerivedBuilder<__LocalEliBase,__TT,__CtorArgs...>; \
  template <class... __InArgs> static SST::ELI::BuilderLibrary<__LocalEliBase,__InArgs...>* \
  getBuilderLibraryTemplate(const std::string& name){ \
    return ::SST::ELI::BuilderDatabase::getLibrary<__LocalEliBase,__InArgs...>(name); \
  } \
  template <class __TT> static bool addDerivedBuilder(const std::string& lib, const std::string& elem){ \
    return Ctor::template add<0,__TT>(lib,elem); \
  } \

#define SST_ELI_DECLARE_CTORS(...) \
  SST_ELI_CTORS_COMMON(ELI_FORWARD_AS_ONE(__VA_ARGS__)) \
  template <class... Args> static bool addBuilder(const std::string& elemlib, const std::string& elem, \
                                           SST::ELI::Builder<__LocalEliBase,Args...>* builder){ \
    return getBuilderLibraryTemplate<Args...>(elemlib)->addBuilder(elem, builder); \
  }

#define SST_ELI_DECLARE_CTORS_EXTERN(...) \
  SST_ELI_CTORS_COMMON(ELI_FORWARD_AS_ONE(__VA_ARGS__))

//VA_ARGS here 
// 0) Base name
// 1) List of ctor args
#define SST_ELI_BUILDER_TYPEDEFS(...) \
  using BaseBuilder = ::SST::ELI::Builder<__VA_ARGS__>; \
  using BuilderLibrary = ::SST::ELI::BuilderLibrary<__VA_ARGS__>; \
  using BuilderLibraryDatabase = ::SST::ELI::BuilderLibraryDatabase<__VA_ARGS__>; \
  template <class __TT> using DerivedBuilder = ::SST::ELI::DerivedBuilder<__TT,__VA_ARGS__>; 

#define SST_ELI_BUILDER_FXNS() \
  static BuilderLibrary* getBuilderLibrary(const std::string& name){ \
    return BuilderLibraryDatabase::getLibrary(name); \
  } \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,builder); \
  }

//I can make some extra using typedefs because I have only a single ctor
#define SST_ELI_DECLARE_CTOR(...) \
  using Ctor = ::SST::ELI::SingleCtor<__LocalEliBase,__VA_ARGS__>; \
  SST_ELI_BUILDER_TYPEDEFS(__LocalEliBase,__VA_ARGS__) \
  SST_ELI_BUILDER_FXNS()

#define SST_ELI_BUILDER_FXNS_EXTERN() \
  static BuilderLibrary* getBuilderLibrary(const std::string& name); \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder);

#define SST_ELI_DECLARE_CTOR_EXTERN(...) \
  using Ctor = ::SST::ELI::SingleCtor<__LocalEliBase,__VA_ARGS__>; \
  SST_ELI_BUILDER_TYPEDEFS(__LocalEliBase,__VA_ARGS__); \
  SST_ELI_BUILDER_FXNS_EXTERN()

#define SST_ELI_DEFINE_CTOR_EXTERN(base) \
  bool base::addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,builder); \
  } \
  base::BuilderLibrary* base::getBuilderLibrary(const std::string& elemlib){ \
    return BuilderLibraryDatabase::getLibrary(elemlib); \
  }

//I can make some extra using typedefs because I have only a single ctor
#define SST_ELI_DECLARE_DEFAULT_CTOR() \
  using Ctor = ::SST::ELI::SingleCtor<__LocalEliBase>; \
  SST_ELI_BUILDER_TYPEDEFS(__LocalEliBase) \
  SST_ELI_BUILDER_FXNS() 

#define SST_ELI_DECLARE_DEFAULT_CTOR_EXTERN() \
  SST_ELI_DEFAULT_CTOR_COMMON() \
  SST_ELI_BUILDER_FXNS_EXTERN()

#define SST_ELI_EXTEND_CTOR() \
  using Ctor = ::SST::ELI::ExtendedCtor<LocalCtor, __ParentEliBase::Ctor>;

#define SST_ELI_SAME_BASE_CTOR() \
  using LocalCtor = __ParentEliBase::Ctor::ChangeBase<__LocalEliBase>; \
  SST_ELI_EXTEND_CTOR() \
  using BaseBuilder = typename __ParentEliBase::BaseBuilder::template ChangeBase<__LocalEliBase>; \
  using BuilderLibrary = __ParentEliBase::BuilderLibrary::ChangeBase<__LocalEliBase>; \
  using BuilderLibraryDatabase = __ParentEliBase::BuilderLibraryDatabase::ChangeBase<__LocalEliBase>; \
  SST_ELI_BUILDER_FXNS()

#define SST_ELI_NEW_BASE_CTOR(...) \
  using LocalCtor = ::SST::ELI::SingleCtor<__LocalEliBase,__VA_ARGS__>; \
  SST_ELI_EXTEND_CTOR() \
  SST_ELI_BUILDER_TYPEDEFS(__LocalEliBase, __VA_ARGS__) \
  SST_ELI_BUILDER_FXNS()

#define SST_ELI_DEFAULT_BASE_CTOR() \
  using LocalCtor = ::SST::ELI::SingleCtor<__LocalEliBase>; \
  SST_ELI_EXTEND_CTOR() \
  SST_ELI_BUILDER_TYPEDEFS(__LocalEliBase)  \
  SST_ELI_BUILDER_FXNS()


#endif

