
#ifndef SST_CORE_FACTORY_INFO_H
#define SST_CORE_FACTORY_INFO_H

#include <oldELI.h>
#include <type_traits>

namespace SST {
namespace ELI {

template <class Base, class... Args>
struct Builder
{
  typedef Base* (*createFxn)(Args...);

  virtual Base* create(Args... ctorArgs) = 0;
};

template <class Base, class... CtorArgs>
class BuilderLibrary
{
 public:
  using BaseBuilder = Builder<Base,CtorArgs...>;

  BaseBuilder* getBuilder(const std::string &name) {
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

  bool addBuilder(const std::string& name, BaseBuilder* fact){
    factories_[name] = fact;
    return true;
  }

 private:
  std::map<std::string, BaseBuilder*> factories_;
};

template <class Base, class... CtorArgs>
class BuilderLibraryDatabase {
 public:
  using Library=BuilderLibrary<Base,CtorArgs...>;
  using BaseFactory=typename Library::BaseBuilder;

  static Library* getLibrary(const std::string& name){
    if (!libraries){
      libraries = std::unique_ptr<std::map<std::string,Library*>>(new std::map<std::string,Library*>);
    }
    auto iter = libraries->find(name);
    if (iter == libraries->end()){
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

template <class Base, class... CtorArgs>
 std::unique_ptr<std::map<std::string,BuilderLibrary<Base,CtorArgs...>*>>
  BuilderLibraryDatabase<Base,CtorArgs...>::libraries;


template <class Base, class T>
struct InstantiateBuilder {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

template <class Base, class T>
 const bool InstantiateBuilder<Base,T>::loaded = Base::Ctor::template add<T>(T::ELI_getLibrary(), T::ELI_getName());

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

template <class Base, class T, class... Args>
struct DerivedBuilder : public Builder<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    return Allocator<Base,T>()(std::forward<Args>(ctorArgs)...);
  }
};

template <class Base, class... Args>
struct DerivedBuilder<Base,OldELITag,Args...> :
  public Builder<Base,Args...>
{
  using typename Builder<Base,Args...>::createFxn;

  DerivedBuilder(createFxn fxn) :
    create_(fxn)
  {
  }

  Base* create(Args... ctorArgs) override {
    return create_(std::forward<Args>(ctorArgs)...);
  }

 private:
  createFxn create_;

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
    return new DerivedBuilder<Base,T,Args...>();
  }

};

template <class Base, class... Args>
struct SingleCtor
{
  template <class T> static bool add(const std::string& elemlib, const std::string& elem){
    //if abstract, force an allocation to generate meaningful errors
    auto* fact = new DerivedBuilder<Base,T,Args...>;
    Base::addBuilder(elemlib,elem,fact);
    return true;
  }
};


template <class Base, class Ctor, class... Ctors>
struct CtorList : public CtorList<Base,Ctors...>
{
  template <class T, int NumValid=0, class U=T>
  static typename std::enable_if<std::is_abstract<U>::value || is_tuple_constructible<U,Ctor>::value, bool>::type
  add(const std::string& elemlib, const std::string& elem){
    //if abstract, force an allocation to generate meaningful errors
    auto* fact = ElementsBuilder<Base,Ctor>::template makeBuilder<U>();
    Base::addBuilder(elemlib,elem,fact);
    return CtorList<Base,Ctors...>::template add<T,NumValid+1>(elemlib,elem);
  }

  template <class T, int NumValid=0, class U=T>
  static typename std::enable_if<!std::is_abstract<U>::value && !is_tuple_constructible<U,Ctor>::value, bool>::type
  add(const std::string& elemlib, const std::string& elem){
    return CtorList<Base,Ctors...>::template add<T,NumValid>(elemlib,elem);
  }

  /**
  //Ctor is a tuple
  template <class... InArgs>
  typename std::enable_if<std::is_convertible<std::tuple<InArgs&&...>, Ctor>::value, Base*>::type
  operator()(const std::string& elemlib, const std::string& name, InArgs&&... args){
    auto* lib = ElementsBuilder<Base,Ctor>::getLibrary(elemlib);
    if (lib){
      auto* fact = lib->getBuilder(name);
      if (fact){
        return fact->create(std::forward<InArgs>(args)...);
      }
      std::cerr << "For library '" << elemlib << "', " << Base::ELI_baseName()
               << " '" << name << "' is not registered with matching constructor"
               << std::endl;
    }
    std::cerr << "Nothing registerd in library '" << elemlib
              << "' for base " << Base::ELI_baseName() << std::endl;
    abort();
  }

  //Ctor is a tuple
  template <class... InArgs>
  typename std::enable_if<!std::is_convertible<std::tuple<InArgs&&...>, Ctor>::value, Base*>::type
  operator()(const std::string& elemlib, const std::string& name, InArgs&&... args){
    //I don't match - pass it up
    return CtorList<Base,Ctors...>::operator()(elemlib, name, std::forward<InArgs>(args)...);
  }
  */

};

template <int NumValid>
struct NoValidConstructorsForDerivedType {
  static constexpr bool atLeastOneValidCtor = true;
};

template <> class NoValidConstructorsForDerivedType<0> {};

template <class Base> struct CtorList<Base,void>
{
  template <class T,int numValidCtors>
  static bool add(const std::string& UNUSED(lib), const std::string& UNUSED(elem)){
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

#define SST_ELI_CTOR_COMMON(...) \
  using Ctor = ::SST::ELI::SingleCtor<__LocalEliBase,__VA_ARGS__>; \
  using BaseBuilder = ::SST::ELI::Builder<__LocalEliBase,__VA_ARGS__>; \
  using BuilderLibrary = ::SST::ELI::BuilderLibrary<__LocalEliBase,__VA_ARGS__>; \
  using BuilderLibraryDatabase = ::SST::ELI::BuilderLibraryDatabase<__LocalEliBase,__VA_ARGS__>; \
  template <class __TT> using DerivedBuilder = ::SST::ELI::DerivedBuilder<__LocalEliBase,__TT,__VA_ARGS__>; \
  template <class __TT> static bool addDerivedBuilder(const std::string& lib, const std::string& elem){ \
    return addBuilder(lib,elem,new DerivedBuilder<__TT>); \
  }

//I can make some extra using typedefs because I have only a single ctor
#define SST_ELI_DECLARE_CTOR(...) \
  SST_ELI_CTOR_COMMON(__VA_ARGS__) \
  static BuilderLibrary* getBuilderLibrary(const std::string& name){ \
    return SST::ELI::BuilderDatabase::getLibrary<__LocalEliBase,__VA_ARGS__>(name); \
  } \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,builder); \
  }

#define SST_ELI_DECLARE_CTOR_EXTERN(...) \
  SST_ELI_CTOR_COMMON(__VA_ARGS__) \
  static BuilderLibrary* getBuilderLibrary(const std::string& name); \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder);

#define SST_ELI_DEFINE_CTOR_EXTERN(base) \
  bool base::addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,builder); \
  } \
  base::BuilderLibrary* base::getBuilderLibrary(const std::string& elemlib){ \
    return BuilderLibraryDatabase::getLibrary(elemlib); \
  }

#define SST_ELI_DEFAULT_CTOR_COMMON() \
  using BaseBuilder = ::SST::ELI::Builder<__LocalEliBase>; \
  using BuilderLibrary = ::SST::ELI::BuilderLibrary<__LocalEliBase>; \
  template <class __TT> using DerivedBuilder = ::SST::ELI::DerivedBuilder<__LocalEliBase,__TT>;

//I can make some extra using typedefs because I have only a single ctor
#define SST_ELI_DECLARE_DEFAULT_CTOR() \
  SST_ELI_DEFAULT_CTOR_COMMON() \
  static BuilderLibrary<>* getBuilderLibrary(const std::string& name){ \
    return SST::ELI::BuilderDatabase::getLibrary<__LocalEliBase>(name); \
  } \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,builder); \
  }

#define SST_ELI_DECLARE_DEFAULT_CTOR_EXTERN() \
  SST_ELI_DEFAULT_CTOR_COMMON() \
  static BuilderLibrary<>* getBuilderLibrary(const std::string& name); \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, BaseBuilder* builder);


#endif

