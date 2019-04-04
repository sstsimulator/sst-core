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
#include <sst/core/elementinfo.h>

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
class ElemLoader;
class SSTElementPythonModule;

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

    
    /**
     * General function for a given base class
     * @param type
     * @param params
     * @param args Constructor arguments
     */
    template <class Base, class... CtorArgs>
    Base* Create(const std::string& type, SST::Params& params, CtorArgs&&... args){
      std::string elemlib, elem;
      std::tie(elemlib, elem) = parseLoadName(type);

      requireLibrary(elemlib);
      std::lock_guard<std::recursive_mutex> lock(factoryMutex);

      auto* lib = ELI::InfoDatabase::getLibrary<Base>(elemlib);
      if (lib){
        auto* info = lib->getInfo(elem);
        if (info){
          auto* builderLib = Base::getBuilderLibrary(elemlib);
          if (builderLib){
            auto* fact = builderLib->getBuilder(elem);
            if (fact){
              params.pushAllowedKeys(info->getParamNames());
              Base* ret = fact->create(std::forward<CtorArgs>(args)...);
              params.popAllowedKeys();
              return ret;
            }
          }
        }
      }
      notFound(Base::ELI_baseName(), type);
      return nullptr;
    }

    /** Instantiate a new Statistic
     * @param comp - Owning component
     * @param type - Fully qualified elementlibname.statisticname type
     * @param statName - Name of the statistic
     * @param statSubId - Name of the sub statistic
     * @param params - Parameters to pass to the Statistics's constructor
     * @param fieldType - Type of data stored in statistic
     */
    template <class T, class... Args>
    Statistics::Statistic<T>* CreateStatistic(std::string type,
                                   BaseComponent* comp, const std::string& statName,
                                   const std::string& stat, Params& params,
                                   Args... args){
      std::string elemlib, elem;
      std::tie(elemlib, elem) = parseLoadName(type);
      // ensure library is already loaded...
      requireLibrary(elemlib);

      auto* lib = ELI::BuilderDatabase::getLibrary<Statistics::Statistic<T>, Args...>(elemlib);
      if (lib){
        auto* fact = lib->getFactory(elem);
        if (fact){
          return fact->create(comp, statName, stat, params, std::forward<Args>(args)...);
        }
      }
      // If we make it to here, component not found
      out.fatal(CALL_INFO, -1,"can't find requested statistic %s.\n ", type.c_str());
      return NULL;
    }


    /** Return Python Module creation function
     * @param name - Fully qualified elementlibname.pythonModName type name
     */
    SSTElementPythonModule* getPythonModule(std::string name);
    /** Checks to see if library exists and can be loaded */
    bool hasLibrary(std::string elemlib);
    void requireLibrary(std::string &elemlib);

    void getLoadedLibraryNames(std::set<std::string>& lib_names);
    void loadUnloadedLibraries(const std::set<std::string>& lib_names);

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
    friend int ::main(int argc, char **argv);

    void notFound(const std::string& baseName, const std::string& type);


    Factory(std::string searchPaths);
    ~Factory();

    Factory();                      // Don't Implement
    Factory(Factory const&);        // Don't Implement
    void operator=(Factory const&); // Don't Implement

    static Factory *instance;

    // find library information for name
    bool findLibrary(std::string name, bool showErrors=true);
    // handle low-level loading of name
    bool loadLibrary(std::string name, bool showErrors=true);

    std::set<std::string> loaded_libraries;

    std::string searchPaths;

    ElemLoader *loader;
    std::string loadingComponentType;

    std::pair<std::string, std::string> parseLoadName(const std::string& wholename);

    std::recursive_mutex factoryMutex;


protected:
   Output &out;
};


} // namespace SST

#endif // SST_CORE_FACTORY_H
