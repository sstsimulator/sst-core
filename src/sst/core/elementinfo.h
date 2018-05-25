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
  documentation in the database.
**************************************************************************/


/**************************************************************************
  Class to check for an ELI_getParams Function
**************************************************************************/
template <class T>
class checkForELI_getParamsFunction
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    typedef const std::vector<ElementInfoParam>& (*functionsig)();

    template <typename F> static Match HasFunction(check<functionsig, &F::ELI_getParams >*);
    template <typename F> static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match) );
};


template<class T>
typename std::enable_if<checkForELI_getParamsFunction<T>::value, const std::vector<ElementInfoParam>& >::type
ELI_templatedGetParams() {
    return T::ELI_getParams();
}

template<class T>
typename std::enable_if<not checkForELI_getParamsFunction<T>::value, std::vector<ElementInfoParam>& >::type
ELI_templatedGetParams() {
    static std::vector<ElementInfoParam> var;
    return var;
}


/**************************************************************************
  Class to check for an ELI_getStatistics Function
**************************************************************************/
template <class T>
class checkForELI_getStatisticsFunction
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    typedef const std::vector<ElementInfoStatistic>& (*functionsig)();

    template <typename F> static Match HasFunction(check<functionsig, &F::ELI_getStatistics>*);
    template <typename F> static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match) );
};

template<class T>
typename std::enable_if<checkForELI_getStatisticsFunction<T>::value, const std::vector<ElementInfoStatistic>& >::type
ELI_templatedGetStatistics() {
    return T::ELI_getStatistics();
}

template<class T>
typename std::enable_if<not checkForELI_getStatisticsFunction<T>::value, const std::vector<ElementInfoStatistic>& >::type
ELI_templatedGetStatistics() {
    static std::vector<ElementInfoStatistic> var;
    return var;
}


/**************************************************************************
  Class to check for an ELI_getPorts Function
**************************************************************************/
template <class T>
class checkForELI_getPortsFunction
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    typedef const std::vector<ElementInfoPort2>& (*functionsig)();

    template <typename F> static Match HasFunction(check<functionsig, &F::ELI_getPorts>*);
    template <typename F> static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match) );
};

template<class T>
typename std::enable_if<checkForELI_getPortsFunction<T>::value, const std::vector<ElementInfoPort2>& >::type
ELI_templatedGetPorts() {
    return T::ELI_getPorts();
}

template<class T>
typename std::enable_if<not checkForELI_getPortsFunction<T>::value, const std::vector<ElementInfoPort2>& >::type
ELI_templatedGetPorts() {
    static std::vector<ElementInfoPort2> var;
    return var;
}


/**************************************************************************
  Class to check for an ELI_getSubComponentSlots Function
**************************************************************************/
template <class T>
class checkForELI_getSubComponentSlotsFunction
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    typedef const std::vector<ElementInfoSubComponentSlot>& (*functionsig)();

    template <typename F> static Match HasFunction(check<functionsig, &F::ELI_getSubComponentSlots>*);
    template <typename F> static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match) );
};

template<class T>
typename std::enable_if<checkForELI_getSubComponentSlotsFunction<T>::value, const std::vector<ElementInfoSubComponentSlot>& >::type
ELI_templatedGetSubComponentSlots() {
    return T::ELI_getSubComponentSlots();
}

template<class T>
typename std::enable_if<not checkForELI_getSubComponentSlotsFunction<T>::value, const std::vector<ElementInfoSubComponentSlot>& >::type
ELI_templatedGetSubComponentSlots() {
    static std::vector<ElementInfoSubComponentSlot> var;
    return var;
}


/**************************************************************************
  Class to check for an ELI_CustomCreate Function for Components
**************************************************************************/
template <class T>
class checkForELI_CustomCreateFunctionforComponent
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    typedef Component* (*functionsig)(ComponentId_t, Params&);

    template <typename F> static Match HasFunction(check<functionsig, &F::ELI_CustomCreate>*);
    template <typename F> static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match) );
};

template<class T>
typename std::enable_if<checkForELI_CustomCreateFunctionforComponent<T>::value, Component* >::type
ELI_templatedCreateforComponent(ComponentId_t id, Params& params) {
    return T::ELI_CustomCreate(id, params);
}

template<class T>
typename std::enable_if<not checkForELI_CustomCreateFunctionforComponent<T>::value, Component* >::type
ELI_templatedCreateforComponent(ComponentId_t id, Params& params) {
    return new T(id, params);
}

/**************************************************************************
  Class to check for an ELI_CustomCreate Function for SubComponents
**************************************************************************/
template <class T>
class checkForELI_CustomCreateFunctionforSubComponent
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    struct FunctionSignature
    {
        typedef SubComponent* (*function)(Component*, Params&);
    };

    template <typename F> static Match HasFunction(check< typename FunctionSignature::function, &F::ELI_CustomCreate>*);
    template <typename F> static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match) );
};

template<class T>
typename std::enable_if<checkForELI_CustomCreateFunctionforSubComponent<T>::value, SubComponent* >::type
ELI_templatedCreateforSubComponent(Component* comp, Params& params) {
    return T::ELI_CustomCreate(comp, params);
}

template<class T>
typename std::enable_if<not checkForELI_CustomCreateFunctionforSubComponent<T>::value, SubComponent* >::type
ELI_templatedCreateforSubComponent(Component* comp, Params& params) {
    return new T(comp, params);
}


#if 0

// This has code for looking for non-static class functions
template <class T>
class checkForCustomCreateComponent
{
    template <typename F, F> struct check;

    typedef char Match;
    typedef long NotMatch;

    // Used for non-static
    // template <typename F>
    // struct FunctionSignature
    // {
    //     typedef Component* (F::*function)(ComponentId_t,Params&);
    // };
        
    struct FunctionSignature
    {
        typedef Component* (*function)(ComponentId_t,Params&);
    };

    // Used for non-static functions
    // template <typename F> static Match HasCustomCreate(check< typename FunctionSignature<F>::function, &F::ELI_Custom_Create >*);
    template <typename F> static Match HasCustomCreate(check< typename FunctionSignature::function, &F::ELI_Custom_Create >*);
    template <typename F> static NotMatch HasCustomCreate(...);

public:
    static bool const value = (sizeof(HasCustomCreate<T>(0)) == sizeof(Match) );
};

#endif


/**************************************************************************
  Classes to support Components
**************************************************************************/


template <class T, unsigned V1, unsigned V2, unsigned V3>
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
        return ELI_templatedCreateforComponent<T>(id,params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return ELI_templatedGetParams<T>(); }
    const std::vector<ElementInfoStatistic>& getValidStats() { return ELI_templatedGetStatistics<T>(); }
    const std::vector<ElementInfoPort2>& getValidPorts() { return ELI_templatedGetPorts<T>(); }
    const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() { return ELI_templatedGetSubComponentSlots<T>(); }
    uint32_t getCategory() { return T::ELI_getCategory(); };
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }

};


// template<class T, int A> const bool SST::ComponentDoc<T, A>::loaded = SST::ElementLibraryDatabase::addComponent(new SST::ComponentDoc<T,T::ELI_getMajorVersion()>());
template<class T, unsigned V1, unsigned V2, unsigned V3> const bool SST::ComponentDoc<T,V1,V2,V3>::loaded = SST::ElementLibraryDatabase::addComponent(new SST::ComponentDoc<T,V1,V2,V3>());


/**************************************************************************
  Classes to support SubComponents
**************************************************************************/

template <class T, unsigned V1, unsigned V2, unsigned V3>
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
        return ELI_templatedCreateforSubComponent<T>(comp,params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() { return T::ELI_getLibrary(); }
    const std::string getName() { return T::ELI_getName(); }
    const std::string getDescription() { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return ELI_templatedGetParams<T>(); }
    const std::vector<ElementInfoStatistic>& getValidStats() { return ELI_templatedGetStatistics<T>(); }
    const std::vector<ElementInfoPort2>& getValidPorts() { return ELI_templatedGetPorts<T>(); }
    const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() { return ELI_templatedGetSubComponentSlots<T>(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template<class T, unsigned V1, unsigned V2, unsigned V3> const bool SubComponentDoc<T,V1,V2,V3>::loaded = ElementLibraryDatabase::addSubComponent(new SubComponentDoc<T,V1,V2,V3>());


/**************************************************************************
  Classes to support Modules
  There's some template metaprogramming to check for the two different
  versions of constructors that can exist.
**************************************************************************/

template <class T, unsigned V1, unsigned V2, unsigned V3>
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
    const std::vector<ElementInfoParam>& getValidParams() { return ELI_templatedGetParams<T>(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template <class T, unsigned V1, unsigned V2, unsigned V3>
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
    const std::vector<ElementInfoParam>& getValidParams() { return ELI_templatedGetParams<T>(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template <class T, unsigned V1, unsigned V2, unsigned V3>
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
    const std::vector<ElementInfoParam>& getValidParams() { return ELI_templatedGetParams<T>(); }
    const std::string getInterface() { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};


// These static functions choose between the custom and not custom
// create versions by looking for ELI_Custom_Create
template<class T, unsigned V1, unsigned V2, unsigned V3>
typename std::enable_if<std::is_constructible<T,Component*,Params&>::value &&
                        std::is_constructible<T,Params&>::value, ModuleElementInfo*>::type
createModuleDoc() {
    return new ModuleDoc<T,V1,V2,V3>();
}

template<class T, unsigned V1, unsigned V2, unsigned V3>
typename std::enable_if<std::is_constructible<T,Component*,Params&>::value &&
                        not std::is_constructible<T,Params&>::value, ModuleElementInfo*>::type
createModuleDoc() {
    return new ModuleDocWithComponent<T,V1,V2,V3>();
}

template<class T, unsigned V1, unsigned V2, unsigned V3>
typename std::enable_if<not std::is_constructible<T,Component*,Params&>::value &&
                        std::is_constructible<T,Params&>::value, ModuleElementInfo*>::type
createModuleDoc() {
    return new ModuleDocWithoutComponent<T,V1,V2,V3>();
}

template<class T, unsigned V1, unsigned V2, unsigned V3> const bool ModuleDoc<T,V1,V2,V3>::loaded = ElementLibraryDatabase::addModule(createModuleDoc<T,V1,V2,V3>());
// template<class T> const bool ModuleDoc<T>::loaded = ElementLibraryDatabase::addModule(new ModuleDoc<T>());


/**************************************************************************
  Classes to support partitioners
**************************************************************************/

template <class T, unsigned V1, unsigned V2, unsigned V3>
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
    const std::vector<int>& getVersion() override { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() override { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() override { return T::ELI_getCompileDate(); }
};

template<class T, unsigned V1, unsigned V2, unsigned V3> const bool PartitionerDoc<T,V1,V2,V3>::loaded = ElementLibraryDatabase::addPartitioner(new PartitionerDoc<T,V1,V2,V3>());


/**************************************************************************
  Classes to support element python modules
**************************************************************************/
template <class T, unsigned V1, unsigned V2, unsigned V3>
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
    const std::vector<int>& getVersion() { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() { return T::ELI_getCompileDate(); }
};

template<class T, unsigned V1, unsigned V2, unsigned V3> T* PythonModuleDoc<T,V1,V2,V3>::instance = NULL;
template<class T, unsigned V1, unsigned V2, unsigned V3> const bool PythonModuleDoc<T,V1,V2,V3>::loaded = ElementLibraryDatabase::addPythonModule(new PythonModuleDoc<T,V1,V2,V3>());



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
        static const std::vector<int> var(SST::SST_ELI_VERSION);      \
        return var; \
    }


#define SST_ELI_REGISTER_COMPONENT(cls,lib,name,version,desc,cat)   \
    bool ELI_isLoaded() {                           \
        return SST::ComponentDoc<cls,SST::SST_ELI_getMajorNumberFromVersion(version),SST::SST_ELI_getMinorNumberFromVersion(version),SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
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

#define SST_ELI_ELEMENT_VERSION(...) {__VA_ARGS__}


#define SST_ELI_DOCUMENT_PARAMS(...)                              \
    static const std::vector<SST::ElementInfoParam>& ELI_getParams() { \
        static std::vector<SST::ElementInfoParam> var = { __VA_ARGS__ } ; \
        return var; \
    }


#define SST_ELI_DOCUMENT_STATISTICS(...)                                \
    static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics() {  \
        static std::vector<SST::ElementInfoStatistic> var = { __VA_ARGS__ } ;  \
        return var; \
    }


#define SST_ELI_DOCUMENT_PORTS(...)                              \
    static const std::vector<SST::ElementInfoPort2>& ELI_getPorts() { \
        static std::vector<SST::ElementInfoPort2> var = { __VA_ARGS__ } ;      \
        return var; \
    }

#define SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(...)                              \
    static const std::vector<SST::ElementInfoSubComponentSlot>& ELI_getSubComponentSlots() { \
    static std::vector<SST::ElementInfoSubComponentSlot> var = { __VA_ARGS__ } ; \
        return var; \
    }


#define SST_ELI_REGISTER_SUBCOMPONENT(cls,lib,name,version,desc,interface)   \
    bool ELI_isLoaded() {                           \
        return SST::SubComponentDoc<cls,SST::SST_ELI_getMajorNumberFromVersion(version),SST::SST_ELI_getMinorNumberFromVersion(version),SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
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
    static const std::vector<int>& ELI_getVersion() { \
        static std::vector<int> var = version ; \
        return var; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()



#define SST_ELI_REGISTER_MODULE(cls,lib,name,version,desc,interface)    \
    bool ELI_isLoaded() {                           \
        return SST::ModuleDoc<cls,SST::SST_ELI_getMajorNumberFromVersion(version),SST::SST_ELI_getMinorNumberFromVersion(version),SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
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
        return interface;                          \
    } \
    static const std::vector<int>& ELI_getVersion() { \
        static std::vector<int> var = version ; \
        return var; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()



#define SST_ELI_REGISTER_PARTITIONER(cls,lib,name,version,desc) \
    bool ELI_isLoaded() { \
        return SST::PartitionerDoc<cls,SST::SST_ELI_getMajorNumberFromVersion(version),SST::SST_ELI_getMinorNumberFromVersion(version),SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
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
    static const std::vector<int>& ELI_getVersion() { \
        static std::vector<int> var = version ; \
        return var; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()


#define SST_ELI_REGISTER_PYTHON_MODULE(cls,lib,version)    \
    bool ELI_isLoaded() { \
        return SST::PythonModuleDoc<cls,SST::SST_ELI_getMajorNumberFromVersion(version),SST::SST_ELI_getMinorNumberFromVersion(version),SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
    } \
    static const std::string ELI_getLibrary() { \
      return lib; \
    } \
    static const std::vector<int>& ELI_getVersion() { \
        static std::vector<int> var = version ; \
        return var; \
    } \
    SST_ELI_INSERT_COMPILE_INFO()


} //namespace SST

#endif // SST_CORE_ELEMENTINFO_H
