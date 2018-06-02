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

#ifndef _SST_CORE_FACTORY_H
#define _SST_CORE_FACTORY_H

#include <sst/core/sst_types.h>

#include <stdio.h>
#include <mutex>

#include <sst/core/params.h>
#include <sst/core/elemLoader.h>
#include <sst/core/element.h>
#include <sst/core/elementinfo.h>
#include <sst/core/model/element_python.h>
#include <sst/core/statapi/statfieldinfo.h>

/* Forward declare for Friendship */
extern int main(int argc, char **argv);

namespace SST {
namespace Statistics {
class StatisticOutput;
class StatisticBase;
}

class Module;
class Component;
class BaseComponent;
class SubComponent;

/**
 * Class for instantiating Components, Links and the like out
 * of element libraries.
 */
class Factory {
public:

    static Factory* getFactory() { return instance; }

    /** Get a list of allowed ports for a given component type.
     * @param type - Name of component in lib.name format
     * @return True if this is a valid portname
     */
    bool isPortNameValid(const std::string &type, const std::string port_name);


    /** Attempt to create a new Component instantiation
     * @param id - The unique ID of the component instantiation
     * @param componentname - The fully qualified elementlibname.componentname type of component
     * @param params - The params to pass to the component's constructor
     * @return Newly created component
     */
    Component* CreateComponent(ComponentId_t id, std::string &componentname,
                               Params& params);

    /** Ensure that an element library containing the required event is loaded
     * @param eventname - The fully qualified elementlibname.eventname type
     */
    void RequireEvent(std::string eventname);

    /** Instantiate a new Module
     * @param type - Fully qualified elementlibname.modulename type
     * @param params - Parameters to pass to the Module's constructor
     */
    Module* CreateModule(std::string type, Params& params);

    /** Instantiate a new Module
     * @param type - Fully qualified elementlibname.modulename type
     * @param comp - Component instance to pass to the Module's constructor
     * @param params - Parameters to pass to the Module's constructor
     */
    Module* CreateModuleWithComponent(std::string type, Component* comp, Params& params);

    /** Instantiate a new Module from within the SST core
     * @param type - Name of the module to load (just modulename, not element.modulename)
     * @param params - Parameters to pass to the module at constructor time
     */
    Module* CreateCoreModule(std::string type, Params& params);

    /** Instantiate a new Module from within the SST core
     * @param type - Name of the module to load (just modulename, not element.modulename)
     * @param params - Parameters to pass to the module at constructor time
     */
    Module* CreateCoreModuleWithComponent(std::string type, Component* comp, Params& params);

    /** Instantiate a new Module
     * @param type - Fully qualified elementlibname.modulename type
     * @param comp - Component instance to pass to the SubComponent's constructor
     * @param params - Parameters to pass to the SubComponent's constructor
     */
    SubComponent* CreateSubComponent(std::string type, Component* comp, Params& params);

    /** Return partitioner function
     * @param name - Fully qualified elementlibname.partitioner type name
     */
    Partition::SSTPartitioner* CreatePartitioner(std::string name, RankInfo total_ranks, RankInfo my_rank, int verbosity);

    /** Return generator function
     * @param name - Fully qualified elementlibname.generator type name
     */
    generateFunction GetGenerator(std::string name);


    /** Instantiate a new Statistic
     * @param comp - Owning component
     * @param type - Fully qualified elementlibname.statisticname type
     * @param statName - Name of the statistic
     * @param statSubId - Name of the sub statistic
     * @param params - Parameters to pass to the Statistics's constructor
     * @param fieldType - Type of data stored in statistic
     */
    Statistics::StatisticBase* CreateStatistic(BaseComponent* comp, const std::string &type,
            const std::string &statName, const std::string &statSubId,
            Params &params, Statistics::StatisticFieldInfo::fieldType_t fieldType);


    /** Return Python Module creation function
     * @param name - Fully qualified elementlibname.pythonModName type name
     */
    // genPythonModuleFunction getPythonModule(std::string name);
    SSTElementPythonModule* getPythonModule(std::string name);
    /** Checks to see if library exists and can be loaded */
    bool hasLibrary(std::string elemlib);
    void requireLibrary(std::string &elemlib);

    void getLoadedLibraryNames(std::set<std::string>& lib_names);
    void loadUnloadedLibraries(const std::set<std::string>& lib_names);

    /** Attempt to create a new Statistic Output instantiation
     * @param statOutputType - The name of the Statistic Output to create (Module Name)
     * @param statOutputParams - The params to pass to the statistic output's constructor
     * @return Newly created Statistic Output
     */
    Statistics::StatisticOutput* CreateStatisticOutput(const std::string& statOutputType, const Params& statOutputParams);

    /** Determine if a SubComponentSlot is defined in a components ElementInfoStatistic
     * @param type - The name of the component/subcomponent
     * @param slotName - The name of the SubComponentSlot 
     * @return True if the SubComponentSlot is defined in the component's ELI
     */
    bool DoesSubComponentSlotExist(const std::string& type, const std::string& slotName);

    /** Determine if a statistic is defined in a components ElementInfoStatistic
     * @param type - The name of the component
     * @param statisticName - The name of the statistic 
     * @return True if the statistic is defined in the component's ElementInfoStatistic
     */
    bool DoesComponentInfoStatisticNameExist(const std::string& type, const std::string& statisticName);

    /** Determine if a statistic is defined in a subcomponents ElementInfoStatistic
     * @param type - The name of the subcomponent
     * @param statisticName - The name of the statistic 
     * @return True if the statistic is defined in the component's ElementInfoStatistic
     */
    bool DoesSubComponentInfoStatisticNameExist(const std::string& type, const std::string& statisticName);

    /** Get the enable level of a statistic defined in the component's ElementInfoStatistic
     * @param componentname - The name of the component
     * @param statisticName - The name of the statistic 
     * @return The Enable Level of the statistic from the ElementInfoStatistic
     */
    uint8_t GetComponentInfoStatisticEnableLevel(const std::string& type, const std::string& statisticName);
    
    /** Get the units of a statistic defined in the component's ElementInfoStatistic
     * @param componentname - The name of the component
     * @param statisticName - The name of the statistic 
     * @return The units string of the statistic from the ElementInfoStatistic
     */
    std::string GetComponentInfoStatisticUnits(const std::string& type, const std::string& statisticName);

private:
    Module* LoadCoreModule_StatisticOutputs(std::string& type, Params& params);

    friend int ::main(int argc, char **argv);


    typedef std::map<std::string, const ElementLibraryInfo*> eli_map_t;

    // Need to keep generator info for now
    typedef std::map<std::string, const ElementInfoGenerator*> eig_map_t;

    Factory(std::string searchPaths);
    ~Factory();

    Factory();                      // Don't Implement
    Factory(Factory const&);        // Don't Implement
    void operator=(Factory const&); // Don't Implement

    static Factory *instance;

    // find library information for name
    const ElementLibraryInfo* findLibrary(std::string name, bool showErrors=true);
    // handle low-level loading of name
    const ElementLibraryInfo* loadLibrary(std::string name, bool showErrors=true);

    eli_map_t loaded_libraries;
    eig_map_t found_generators;

    std::string searchPaths;

    ElemLoader *loader;
    std::string loadingComponentType;

    std::pair<std::string, std::string> parseLoadName(const std::string& wholename);

    std::recursive_mutex factoryMutex;


protected:
   Output &out;
};



/*******************************************************
   Classes to provide backward compatibility with the
   old ELI
*******************************************************/

/**************************************************************************
  Class to support Components
**************************************************************************/
class ComponentDocOldELI : public ComponentElementInfo {

private:
    std::string library;
    std::string name;
    std::string description;

    std::vector<ElementInfoParam> valid_params;
    std::vector<ElementInfoStatistic> valid_stats;
    std::vector<ElementInfoPort2> valid_ports;
    std::vector<ElementInfoSubComponentSlot> subcomp_slots;

    uint32_t category;

    componentAllocate alloc;
    
public:

    ComponentDocOldELI(const std::string& library, const ElementInfoComponent* component) :
        ComponentElementInfo(),
        library(library),
        name(component->name),
        description(component->description),
        category(component->category),
        alloc(component->alloc)
        
    {
        const ElementInfoParam *p = component->params;
        while ( NULL != p && NULL != p->name ) {
            valid_params.emplace_back(*p);
            p++;
        }
        
        const ElementInfoPort *po = component->ports;
        while ( NULL != po && NULL != po->name ) {
            valid_ports.emplace_back(po);
            po++;
        }
        
        const ElementInfoStatistic *s = component->stats;
        while ( NULL != s && NULL != s->name ) {
            valid_stats.emplace_back(*s);
            s++;
        }
        
        const ElementInfoSubComponentSlot *ss = component->subComponents;
        while ( NULL != ss && NULL != ss->name ) {
            subcomp_slots.emplace_back(*ss);
            ss++;
        }
        
        initialize_allowedKeys();
        initialize_portnames();
        initialize_statnames();
    }
    
    Component* create(ComponentId_t id, Params& params) {
        return (*alloc)(id,params);
    }

    const std::string getLibrary() { return library; }
    const std::string getName() { return name; }
    const std::string getDescription() { return description; }
    const std::vector<ElementInfoParam>& getValidParams() { return valid_params; }
    const std::vector<ElementInfoStatistic>& getValidStats() { return valid_stats; }
    const std::vector<ElementInfoPort2>& getValidPorts() { return valid_ports; }
    const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() { return subcomp_slots; }

    uint32_t getCategory() { return category; }
    const std::vector<int>& getELICompiledVersion() { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::vector<int>& getVersion() { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::string getCompileFile() { return "UNKNOWN"; }
    const std::string getCompileDate() { return "UNKNOWN"; }
};


/**************************************************************************
  Class to support SubComponents
**************************************************************************/

class SubComponentDocOldELI : public SubComponentElementInfo {
private:

    std::string library;
    std::string name;
    std::string description;

    std::vector<ElementInfoParam> valid_params;
    std::vector<ElementInfoStatistic> valid_stats;
    std::vector<ElementInfoPort2> valid_ports;
    std::vector<ElementInfoSubComponentSlot> subcomp_slots;

    std::string interface;
    subcomponentAllocate alloc;

public:
    
    SubComponentDocOldELI(const std::string& library, const ElementInfoSubComponent* component) :
        SubComponentElementInfo(),
        library(library),
        name(component->name),
        description(component->description),
        interface(component->provides),
        alloc(component->alloc)

    {

        const ElementInfoParam *p = component->params;
        while ( NULL != p && NULL != p->name ) {
            valid_params.emplace_back(*p);
            p++;
        }
        
        const ElementInfoPort *po = component->ports;
        while ( NULL != po && NULL != po->name ) {
            valid_ports.emplace_back(po);
            po++;
        }
        
        const ElementInfoStatistic *s = component->stats;
        while ( NULL != s && NULL != s->name ) {
            valid_stats.emplace_back(*s);
            s++;
        }
        
        const ElementInfoSubComponentSlot *ss = component->subComponents;
        while ( NULL != ss && NULL != ss->name ) {
            subcomp_slots.emplace_back(*ss);
            ss++;
        }
        
        
        initialize_allowedKeys();
        initialize_portnames();
        initialize_statnames();
    }

    SubComponent* create(Component* comp, Params& params) {
        return (*alloc)(comp,params);
    }

    const std::string getLibrary() { return library; }
    const std::string getName() { return name; }
    const std::string getDescription() { return description; }
    const std::vector<ElementInfoParam>& getValidParams() { return valid_params; }
    const std::vector<ElementInfoStatistic>& getValidStats() { return valid_stats; }
    const std::vector<ElementInfoPort2>& getValidPorts() { return valid_ports; }
    const std::vector<ElementInfoSubComponentSlot>& getSubComponentSlots() { return subcomp_slots; }
    const std::string getInterface() { return interface; }

    const std::vector<int>& getELICompiledVersion() { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::vector<int>& getVersion() { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::string getCompileFile() { return "UNKNOWN"; }
    const std::string getCompileDate() { return "UNKNOWN"; }

};


/**************************************************************************
  Class to support Modules
**************************************************************************/

class ModuleDocOldELI : public ModuleElementInfo {
private:

    std::string library;
    std::string name;
    std::string description;

    std::vector<ElementInfoParam> valid_params;

    std::string interface;

    moduleAllocate alloc;
    moduleAllocateWithComponent allocWithComponent;
    
public:
    
    ModuleDocOldELI(const std::string& library, const ElementInfoModule* module) :
        ModuleElementInfo(),
        library(library),
        name(module->name),
        description(module->description),
        alloc(module->alloc),
        allocWithComponent(module->alloc_with_comp)
    {

        const ElementInfoParam *p = module->params;
        while ( NULL != p && NULL != p->name ) {
            valid_params.emplace_back(*p);
            p++;
        }
        
        initialize_allowedKeys();
    }

    Module* create(Component* comp, Params& params) {
        return (*allocWithComponent)(comp,params);
    }

    Module* create(Params& params) {
        return (*alloc)(params);
    }

    const std::string getLibrary() { return library; }
    const std::string getName() { return name; }
    const std::string getDescription() { return description; }
    const std::vector<ElementInfoParam>& getValidParams() { return valid_params; }
    const std::string getInterface() { return interface; }

    const std::vector<int>& getELICompiledVersion() { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::vector<int>& getVersion() { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::string getCompileFile() { return "UNKNOWN"; }
    const std::string getCompileDate() { return "UNKNOWN"; }

};


/**************************************************************************
  Classes to support partitioners
**************************************************************************/

class PartitionerDocOldELI : public PartitionerElementInfo {
private:

    std::string library;
    std::string name;
    std::string description;

    partitionFunction alloc;
    
public:

    PartitionerDocOldELI(const std::string& library, const ElementInfoPartitioner* part) :
        PartitionerElementInfo(),
        library(library),
        name(part->name),
        description(part->description),
        alloc(part->func)
    {
    }
    
    Partition::SSTPartitioner* create(RankInfo total_ranks, RankInfo my_rank, int verbosity) override {
        return (*alloc)(total_ranks,my_rank,verbosity);
    }
    
    const std::string getLibrary() override { return library; }
    const std::string getName() override { return name; }
    const std::string getDescription() override { return description; }

    const std::vector<int>& getELICompiledVersion() override { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::vector<int>& getVersion() override { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::string getCompileFile() override { return "UNKNOWN"; }
    const std::string getCompileDate() override { return "UNKNOWN"; }
};

/**************************************************************************
  Class to support element python modules
**************************************************************************/
class PythonModuleDocOldELI : public PythonModuleElementInfo {
private:

    // Only need to create one of these
    static SSTElementPythonModuleOldELI* instance;
    
    std::string library;
    genPythonModuleFunction alloc;
    
public:

    PythonModuleDocOldELI(const std::string& library, const genPythonModuleFunction func) :
        PythonModuleElementInfo(),
        library(library),
        alloc(func)
    {
    }

    SSTElementPythonModule* create() override {
        // return new T(getLibrary());
        if( instance == NULL ) instance = new SSTElementPythonModuleOldELI(alloc);
        return instance; 
    }
    
    const std::string getLibrary() override { return library; }

    const std::vector<int>& getELICompiledVersion() override { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::vector<int>& getVersion() override { static std::vector<int> vec = {-1, -1, -1};
        return vec; }
    const std::string getCompileFile() override { return "UNKNOWN"; }
    const std::string getCompileDate() override { return "UNKNOWN"; }
};


} // namespace SST

#endif // SST_CORE_FACTORY_H
