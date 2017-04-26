// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
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

#include <string>
#include <vector>

#include <sst/core/elibase.h>

namespace SST {
class Component;
class Module;
class SubComponent;
namespace Partition {
    class SSTPartitioner;
}
class RankInfo;
class SSTElementPythonModule;

/****************************************************
   Base classes for templated documentation classes
*****************************************************/

const std::vector<int> SST_ELI_VERSION = {0, 9, 0};

class BaseElementInfo {

public:
    virtual const std::string getLibrary() = 0;
    virtual const std::string getDescription() = 0;
    virtual const std::string getName() = 0;
    virtual const std::vector<int>& getVersion() = 0;
    virtual const std::string getCompileFile() = 0;
    virtual const std::string getCompileDate() = 0;
    
    virtual const std::vector<int>& getELICompiledVersion() = 0;

    std::string getELIVersionString();
};

class BaseParamsElementInfo : public BaseElementInfo {

protected:
    Params::KeySet_t allowedKeys;
    
    void initialize_allowedKeys();
    
public:

    virtual const std::vector<ElementInfoParam>& getValidParams() = 0;

    const Params::KeySet_t& getParamNames() { return allowedKeys; }

    std::string getParametersString();

};

class BaseComponentElementInfo : public BaseParamsElementInfo {

protected:
    std::vector<std::string> portnames;
    std::vector<std::string> statnames;
    
    void initialize_portnames();
    void initialize_statnames();
    
public:

    virtual const std::vector<ElementInfoPort2>& getValidPorts() = 0;
    virtual const std::vector<ElementInfoStatistic>& getValidStats() = 0;
    virtual const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() = 0;

    const std::vector<std::string>& getPortnames() { return portnames; }
    const std::vector<std::string>& getStatnames() { return statnames; }

    std::string getStatisticsString();
    std::string getPortsString();
    std::string getSubComponentSlotString();
    
};


class ComponentElementInfo : public BaseComponentElementInfo {

public:

    ComponentElementInfo() {  }
    
    virtual Component* create(ComponentId_t id, Params& params) = 0;
    virtual uint32_t getCategory() = 0;
    
    std::string toString();
};

class SubComponentElementInfo : public BaseComponentElementInfo {

public:

    virtual SubComponent* create(Component* comp, Params& params) = 0;
    virtual const std::string getInterface() = 0;

    std::string toString();
};


class ModuleElementInfo : public BaseParamsElementInfo {

protected:

public:
    virtual Module* create(Component* UNUSED(comp), Params& UNUSED(params)) { /* Need to print error */ return NULL; }
    virtual Module* create(Params& UNUSED(params)) { /* Need to print error */ return NULL; }
    virtual const std::string getInterface() = 0;
    
    std::string toString();
};


class PartitionerElementInfo : public BaseElementInfo {
public:
    virtual Partition::SSTPartitioner* create(RankInfo total_ranks, RankInfo my_rank, int verbosity) = 0;
    
    std::string toString();
};

class PythonModuleElementInfo : public BaseElementInfo {
public:
    virtual SSTElementPythonModule* create() = 0;
    const std::string getDescription()  { return getLibrary() + " python module"; };
    const std::string getName() { return getLibrary(); }    
};
 

class LibraryInfo {
public:
    std::map<std::string,ComponentElementInfo*> components;
    std::map<std::string,SubComponentElementInfo*> subcomponents;
    std::map<std::string,ModuleElementInfo*> modules;
    std::map<std::string,PartitionerElementInfo*> partitioners;
    PythonModuleElementInfo* python_module;

    LibraryInfo() :
        python_module(NULL) {}
    
    
    BaseComponentElementInfo* getComponentOrSubComponent(const std::string &name) {
        BaseComponentElementInfo *bcei = getComponent(name);
        if ( !bcei )
            bcei = getSubComponent(name);
        return bcei;
    }

    ComponentElementInfo* getComponent(const std::string &name) {
        if ( components.count(name) == 0 ) return NULL;
        return components[name];
    }

    SubComponentElementInfo* getSubComponent(const std::string &name) {
        if ( subcomponents.count(name) == 0 ) return NULL;
        return subcomponents[name];
    }
    
    ModuleElementInfo* getModule(const std::string &name) {
        if ( modules.count(name) == 0 ) return NULL;
        return modules[name];
    }
    
    PartitionerElementInfo* getPartitioner(const std::string &name) {
        if ( partitioners.count(name) == 0 ) return NULL;
        return partitioners[name];
    }
    
    PythonModuleElementInfo* getPythonModule() {
        return python_module;
    }
    
    std::string toString();
};


class ElementLibraryDatabase {
private:
    // Database
    static std::map<std::string,LibraryInfo*> libraries;

    static LibraryInfo* getLibrary(const std::string &library) {
        if ( libraries.count(library) == 0 ) {
            libraries[library] = new LibraryInfo;
        }
        return libraries[library];
    }
    
public:
    static bool addComponent(ComponentElementInfo* comp) {
        LibraryInfo* library = getLibrary(comp->getLibrary());
        library->components[comp->getName()] = comp;
        return true;
    }

    static bool addSubComponent(SubComponentElementInfo* comp) {
        LibraryInfo* library = getLibrary(comp->getLibrary());
        library->subcomponents[comp->getName()] = comp;
        return true;
    }

    static bool addModule(ModuleElementInfo* comp) {
        LibraryInfo* library = getLibrary(comp->getLibrary());
        library->modules[comp->getName()] = comp;
        return true;
    }

    static bool addPartitioner(PartitionerElementInfo* part) {
        LibraryInfo* library = getLibrary(part->getLibrary());
        library->partitioners[part->getName()] = part;
        return true;
    }

    static bool addPythonModule(PythonModuleElementInfo* pymod) {
        LibraryInfo* library = getLibrary(pymod->getLibrary());
        if ( library->python_module == NULL ) {
            library->python_module = pymod;
        }
        else {
            // need to fatal
        }
        return true;
    }

    static std::string toString();

    static LibraryInfo* getLibraryInfo(const std::string &library) {
        if ( libraries.count(library) == 0 ) return NULL;
        return libraries[library];
    }    
};



/**************************************************************************
  Templated classes that are used to actually implement the element info
  documenation in the database.
**************************************************************************/


/**************************************************************************
  Classes to support Components
**************************************************************************/


template <class T>
class ComponentDoc : public ComponentElementInfo {
private:
    static const bool loaded;
    
public:

    ComponentDoc() : ComponentElementInfo() {
        initialize_allowedKeys();
        initialize_portnames();
        initialize_statnames();
    }
    
    Component* create(ComponentId_t id, Params& params) {
        // return new T(id, params);
        return T::ELI_create(id,params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return T::ELI_getParams(); }
    const std::vector<ElementInfoStatistic>& getValidStats() { return T::ELI_getStatistics(); }
    const std::vector<ElementInfoPort2>& getValidPorts() { return T::ELI_getPorts(); }
    const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() { return T::ELI_getSubComponentSlots(); }
    uint32_t getCategory() { return T::ELI_getCategory(); };
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { return T::ELI_getVersion(); }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};


template<class T> const bool ComponentDoc<T>::loaded = ElementLibraryDatabase::addComponent(new ComponentDoc<T>());


/**************************************************************************
  Classes to support SubComponents
**************************************************************************/

template <class T>
class SubComponentDoc : public SubComponentElementInfo {
private:
    static const bool loaded;

public:
    
    SubComponentDoc() : SubComponentElementInfo() {
        initialize_allowedKeys();
        initialize_portnames();
        initialize_statnames();
    }

    SubComponent* create(Component* comp, Params& params) {
        // return new T(comp,params);
        return T::ELI_create(comp,params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return T::ELI_getParams(); }
    const std::vector<ElementInfoStatistic>& getValidStats() { return T::ELI_getStatistics(); }
    const std::vector<ElementInfoPort2>& getValidPorts() { return T::ELI_getPorts(); }
    const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() { return T::ELI_getSubComponentSlots(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { return T::ELI_getVersion(); }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template<class T> const bool SubComponentDoc<T>::loaded = ElementLibraryDatabase::addSubComponent(new SubComponentDoc<T>());


/**************************************************************************
  Classes to support Modules
  There's some template metaprogramming to check for the two different
  versions of constructors that can exist.
**************************************************************************/

template <class T>
class ModuleDoc : public ModuleElementInfo {
private:
    static const bool loaded;

public:
    
    ModuleDoc() : ModuleElementInfo() {
        initialize_allowedKeys();
    }

    Module* create(Component* comp, Params& params) {
        return new T(comp,params);
    }

    Module* create(Params& params) {
        return new T(params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return T::ELI_getParams(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { return T::ELI_getVersion(); }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template <class T>
class ModuleDocWithComponent : public ModuleElementInfo {
private:
    static const bool loaded;

public:
    
    ModuleDocWithComponent() : ModuleElementInfo() {
        initialize_allowedKeys();
    }

    Module* create(Component* comp, Params& params) {
        return new T(comp,params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return T::ELI_getParams(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { return T::ELI_getVersion(); }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template <class T>
class ModuleDocWithoutComponent : public ModuleElementInfo {
private:
    static const bool loaded;

public:
    
    ModuleDocWithoutComponent() : ModuleElementInfo() {
        initialize_allowedKeys();
    }

    Module* create(Params& params) {
        return new T(params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return T::ELI_getParams(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { return T::ELI_getVersion(); }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};


// These static functions choose between the custom and not custom
// create versions by looking for ELI_Custom_Create
template<class T>
typename std::enable_if<std::is_constructible<T,Component*,Params&>::value &&
                        std::is_constructible<T,Params&>::value, ModuleElementInfo*>::type
createModuleDoc() {
    return new ModuleDoc<T>();
}

template<class T>
typename std::enable_if<std::is_constructible<T,Component*,Params&>::value &&
                        not std::is_constructible<T,Params&>::value, ModuleElementInfo*>::type
createModuleDoc() {
    return new ModuleDocWithComponent<T>();
}

template<class T>
typename std::enable_if<not std::is_constructible<T,Component*,Params&>::value &&
                        std::is_constructible<T,Params&>::value, ModuleElementInfo*>::type
createModuleDoc() {
    return new ModuleDocWithoutComponent<T>();
}

template<class T> const bool ModuleDoc<T>::loaded = ElementLibraryDatabase::addModule(createModuleDoc<T>());
// template<class T> const bool ModuleDoc<T>::loaded = ElementLibraryDatabase::addModule(new ModuleDoc<T>());


/**************************************************************************
  Classes to support partitioners
**************************************************************************/

template <class T>
class PartitionerDoc : PartitionerElementInfo {
private:
    static const bool loaded;

public:
    
    virtual Partition::SSTPartitioner* create(RankInfo total_ranks, RankInfo my_rank, int verbosity) override {
        return new T(total_ranks,my_rank,verbosity);
    }
    
    static bool isLoaded() { return loaded; }
    const std::string getDescription() override { return T::ELI_getDescription(); }
    const std::string getName() override { return T::ELI_getName(); }
    const std::string getLibrary() override { return T::ELI_getLibrary(); }
    const std::vector<int>& getELICompiledVersion() override { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() override { return T::ELI_getVersion(); }
    const std::string getCompileFile() override { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() override { return T::ELI_getCompileDate(); }
};

template<class T> const bool PartitionerDoc<T>::loaded = ElementLibraryDatabase::addPartitioner(new PartitionerDoc<T>());


/**************************************************************************
  Classes to support element python modules
**************************************************************************/
template <class T>
class PythonModuleDoc : public PythonModuleElementInfo {
private:
    static const bool loaded;
    // Only need to create one of these
    static T* instance;
    
public:

    SSTElementPythonModule* create() {
        // return new T(getLibrary());
        if( instance == NULL ) instance = new T(getLibrary());
        return instance; 
    }
    
    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { return T::ELI_getVersion(); }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template<class T> T* PythonModuleDoc<T>::instance = NULL;
template<class T> const bool PythonModuleDoc<T>::loaded = ElementLibraryDatabase::addPythonModule(new PythonModuleDoc<T>());

/**************************************************************************
  Macros used by elements to add element documentation
**************************************************************************/

#define SST_ELI_INSERT_COMPILE_INFO() \
    static const std::string& ELI_getCompileDate() { \
        static std::string time = __TIME__; \
        static std::string date = __DATE__; \
        static std::string date_time = date + " " + time; \
        return date_time;                                 \
    } \
    static const std::string ELI_getCompileFile() { \
        return __FILE__; \
    } \
    static const std::vector<int>& ELI_getELICompiledVersion() { \
        static const std::vector<int> var(SST_ELI_VERSION);      \
        return var; \
    } \
    static const std::vector<int>& ELI_getVersion() { \
        static const std::vector<int> var = {1}; \
        return var; \
    }
// This last is only needed here until we add the version macro to all the elements


#define SST_ELI_REGISTER_COMPONENT_CUSTOM_CREATE(cls,lib,name,desc,cat)   \
    friend class ComponentDoc<cls>; \
    bool ELI_isLoaded() {                           \
        return ComponentDoc<cls>::isLoaded();     \
    } \
    static const std::string ELI_getLibrary() { \
      return lib; \
    } \
    static const std::string ELI_getName() { \
      return name; \
    } \
    static const std::string ELI_getDescription() {  \
      return desc; \
    } \
    static const uint32_t ELI_getCategory() {  \
      return cat; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()

#define SST_ELI_REGISTER_COMPONENT(cls,lib,name,desc,cat) \
    static Component* ELI_create(ComponentId_t id, Params& params) { \
      return new cls(id,params); \
    } \
    SST_ELI_REGISTER_COMPONENT_CUSTOM_CREATE(cls,lib,name,desc,cat)


// For now, just an empty macro.  After adding to elements, will put
// the real one in.  The required function is in a different macro for
// now.
#define SST_ELI_DOCUMENT_VERSION(major,minor,tertiary)
// #define SST_ELI_DOCUMENT_VERSION(major,minor,tertiary)  \
//     static const std::vector<int>& ELI_getVersion() { \
//         static std::vector<int> var = { major, minor, tertiary } ; \
//         return var; \
//     }

#define SST_ELI_DOCUMENT_PARAMS(...)                              \
    static const std::vector<ElementInfoParam>& ELI_getParams() { \
        static std::vector<ElementInfoParam> var = { __VA_ARGS__ } ; \
        return var; \
    }


#define SST_ELI_DOCUMENT_STATISTICS(...)                                \
    static const std::vector<ElementInfoStatistic>& ELI_getStatistics() {  \
        static std::vector<ElementInfoStatistic> var = { __VA_ARGS__ } ;  \
        return var; \
    }


#define SST_ELI_DOCUMENT_PORTS(...)                              \
    static const std::vector<ElementInfoPort2>& ELI_getPorts() { \
        static std::vector<ElementInfoPort2> var = { __VA_ARGS__ } ;      \
        return var; \
    }

#define SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(...)                              \
    static const std::vector<ElementInfoSubComponentSlot>& ELI_getSubComponentSlots() { \
    static std::vector<ElementInfoSubComponentSlot> var = { __VA_ARGS__ } ; \
        return var; \
    }


#define SST_ELI_REGISTER_SUBCOMPONENT_CUSTOM_CREATE(cls,lib,name,desc,interface)   \
    friend class SubComponentDoc<cls>; \
    bool ELI_isLoaded() {                           \
      return SubComponentDoc<cls>::isLoaded(); \
    } \
    static const std::string ELI_getLibrary() { \
      return lib; \
    } \
    static const std::string ELI_getName() { \
      return name; \
    } \
    static const std::string ELI_getDescription() {  \
      return desc; \
    } \
    static const std::string ELI_getInterface() {  \
      return interface; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()

#define SST_ELI_REGISTER_SUBCOMPONENT(cls,lib,name,desc,interface)   \
    static SubComponent* ELI_create(Component* comp, Params& params) { \
    return new cls(comp,params); \
    } \
    SST_ELI_REGISTER_SUBCOMPONENT_CUSTOM_CREATE(cls,lib,name,desc,interface)


#define SST_ELI_REGISTER_MODULE(cls,lib,name,desc,interface) \
    friend class ModuleDoc<cls>; \
    bool ELI_isLoaded() {                           \
      return ModuleDoc<cls>::isLoaded(); \
    } \
    static const std::string ELI_getLibrary() { \
      return lib; \
    } \
    static const std::string ELI_getName() { \
      return name; \
    } \
    static const std::string ELI_getDescription() {  \
      return desc; \
    } \
    static const std::string ELI_getInterface() {  \
      return interface; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()



#define SST_ELI_REGISTER_PARTITIONER(cls,lib,name,desc) \
    friend class PartitionerDoc<cls>; \
    bool ELI_isLoaded() { \
      return PartitionerDoc<cls>::isLoaded(); \
    } \
    static const std::string ELI_getLibrary() { \
      return lib; \
    } \
    static const std::string ELI_getName() { \
      return name; \
    } \
    static const std::string ELI_getDescription() {  \
      return desc; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()
    


#define SST_ELI_REGISTER_PYTHON_MODULE(cls,lib) \
    friend class PythonModuleDoc<cls>; \
    bool ELI_isLoaded() { \
      return PythonModuleDoc<cls>::isLoaded(); \
    } \
    static const std::string ELI_getLibrary() { \
      return lib; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()


} //namespace SST

#endif // SST_CORE_ELEMENTINFO_H
