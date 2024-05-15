// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_FACTORY_H
#define SST_CORE_FACTORY_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/params.h"
#include "sst/core/sst_types.h"
#include "sst/core/sstpart.h"

#include <mutex>
#include <stdio.h>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);

namespace SST {
namespace Statistics {
class StatisticOutput;
class StatisticBase;
} // namespace Statistics

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
class Factory
{
public:
    static Factory* getFactory() { return instance; }

    /** Get a list of allowed ports for a given component type.
     * @param type - Name of component in lib.name format
     * @return True if this is a valid portname
     */
    bool isPortNameValid(const std::string& type, const std::string& port_name);

    /** Get a list of allowed param keys for a given component type.
     * @param type - Name of component in lib.name format
     * @return True if this is a valid portname
     */
    const Params::KeySet_t& getParamNames(const std::string& type);

    /** Attempt to create a new Component instantiation
     * @param id - The unique ID of the component instantiation
     * @param componentname - The fully qualified elementlibname.componentname type of component
     * @param params - The params to pass to the component's constructor
     * @return Newly created component
     */
    Component* CreateComponent(ComponentId_t id, const std::string& componentname, Params& params);

    /** Ensure that an element library containing the required event is loaded
     * @param eventname - The fully qualified elementlibname.eventname type
     */
    void RequireEvent(const std::string& eventname);

    bool doesSubComponentExist(const std::string& type);

    /** Return partitioner function
     * @param name - Fully qualified elementlibname.partitioner type name
     */
    Partition::SSTPartitioner*
    CreatePartitioner(const std::string& name, RankInfo total_ranks, RankInfo my_rank, int verbosity);

    /**
       Check to see if a given element type is loadable with a particular API
       @param name - Name of element to check in lib.name format
       @return True if loadable as the API specified as the template parameter
     */
    template <class Base>
    bool isSubComponentLoadableUsingAPI(const std::string& type)
    {
        std::string elemlib, elem;
        std::tie(elemlib, elem) = parseLoadName(type);

        requireLibrary(elemlib);
        std::lock_guard<std::recursive_mutex> lock(factoryMutex);

        auto* lib = ELI::InfoDatabase::getLibrary<Base>(elemlib);
        if ( lib ) {
            auto  map  = lib->getMap();
            auto* info = lib->getInfo(elem);
            if ( info ) {
                auto* builderLib = Base::getBuilderLibrary(elemlib);
                if ( builderLib ) {
                    auto* fact = builderLib->getBuilder(elem);
                    if ( fact ) { return true; }
                }
            }
        }
        return false;
    }

    template <class Base, int index, class InfoType>
    const InfoType& getSimpleInfo(const std::string& type)
    {
        static InfoType invalid_ret;
        std::string     elemlib, elem;
        std::tie(elemlib, elem) = parseLoadName(type);

        std::stringstream err_os;
        requireLibrary(elemlib, err_os);
        std::lock_guard<std::recursive_mutex> lock(factoryMutex);

        auto* lib = ELI::InfoDatabase::getLibrary<Base>(elemlib);
        if ( lib ) {
            auto* info = lib->getInfo(elem);
            if ( info ) {
                // Need to cast this to a ProvidesSimpleInfo to get
                // the templated data
                auto* cast_info = dynamic_cast<ELI::ProvidesSimpleInfo<index, InfoType>*>(info);
                if ( cast_info ) { return cast_info->getSimpleInfo(); }
            }
        }
        // notFound(Base::ELI_baseName(), type, err_os.str());
        return invalid_ret;
    }

    /**
     * General function to create a given base class.
     *
     * @param type
     * @param params
     * @param args Constructor arguments
     */
    template <class Base, class... CtorArgs>
    Base* Create(const std::string& type, CtorArgs&&... args)
    {
        std::string elemlib, elem;
        std::tie(elemlib, elem) = parseLoadName(type);

        std::stringstream err_os;
        requireLibrary(elemlib, err_os);
        std::lock_guard<std::recursive_mutex> lock(factoryMutex);

        auto* lib = ELI::InfoDatabase::getLibrary<Base>(elemlib);
        if ( lib ) {
            auto* info = lib->getInfo(elem);
            if ( info ) {
                auto* builderLib = Base::getBuilderLibrary(elemlib);
                if ( builderLib ) {
                    auto* fact = builderLib->getBuilder(elem);
                    if ( fact ) {
                        Base* ret = fact->create(std::forward<CtorArgs>(args)...);
                        return ret;
                    }
                }
            }
        }
        notFound(Base::ELI_baseName(), type, err_os.str());
        return nullptr;
    }

    template <class T, class... ARGS>
    T* CreateProfileTool(const std::string& type, ARGS... args)
    {
        return Factory::getFactory()->Create<T>(type, args...);
    }

    /**
     * General function to create a given base class.  This version
     * should be used if a Params object is being passed in and you
     * need the system to populate the allowed keys for the object.
     * If you don't need the allowed keys to be populated, just use
     * the Create() bunction.
     *
     * @param type
     * @param params
     * @param args Constructor arguments
     */
    template <class Base, class... CtorArgs>
    Base* CreateWithParams(const std::string& type, SST::Params& params, CtorArgs&&... args)
    {
        std::string elemlib, elem;
        std::tie(elemlib, elem) = parseLoadName(type);

        std::stringstream err_os;
        requireLibrary(elemlib, err_os);
        std::lock_guard<std::recursive_mutex> lock(factoryMutex);

        auto* lib = ELI::InfoDatabase::getLibrary<Base>(elemlib);
        if ( lib ) {
            auto  map  = lib->getMap();
            auto* info = lib->getInfo(elem);
            if ( info ) {
                auto* builderLib = Base::getBuilderLibrary(elemlib);
                if ( builderLib ) {
                    auto* fact = builderLib->getBuilder(elem);
                    if ( fact ) {
                        params.pushAllowedKeys(info->getParamNames());
                        Base* ret = fact->create(std::forward<CtorArgs>(args)...);
                        params.popAllowedKeys();
                        return ret;
                    }
                }
            }
        }
        notFound(Base::ELI_baseName(), type, err_os.str());
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
    Statistics::Statistic<T>* CreateStatistic(
        const std::string& type, BaseComponent* comp, const std::string& statName, const std::string& stat,
        Params& params, Args... args)
    {
        std::string elemlib, elem;
        std::tie(elemlib, elem) = parseLoadName(type);
        // ensure library is already loaded...
        std::stringstream sstr;
        requireLibrary(elemlib, sstr);

        auto* lib = ELI::BuilderDatabase::getLibrary<Statistics::Statistic<T>, Args...>(elemlib);
        if ( lib ) {
            auto* fact = lib->getFactory(elem);
            if ( fact ) { return fact->create(comp, statName, stat, params, std::forward<Args>(args)...); }
        }
        // If we make it to here, component not found
        out.fatal(CALL_INFO, -1, "can't find requested statistic %s.\n%s\n", type.c_str(), sstr.str().c_str());
        return nullptr;
    }

    /** Return Python Module creation function
     * @param name - Fully qualified elementlibname.pythonModName type name
     */
    SSTElementPythonModule* getPythonModule(const std::string& name);

    /**
     * @brief hasLibrary Checks to see if library exists and can be loaded
     * @param elemlib
     * @param err_os Stream to print error messages to
     * @return whether the library was found
     */
    bool hasLibrary(const std::string& elemlib, std::ostream& err_os);
    void requireLibrary(const std::string& elemlib, std::ostream& err_os);
    /**
     * @brief requireLibrary Throws away error messages
     * @param elemlib
     */
    void requireLibrary(const std::string& elemlib);

    void getLoadedLibraryNames(std::set<std::string>& lib_names);
    void loadUnloadedLibraries(const std::set<std::string>& lib_names);

    const std::string& getSearchPaths();

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

    const std::vector<std::string>& GetValidStatistics(const std::string& compType);

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

    /** Get a list of allowed ports for a given component type.
     * @param type - Type of component in lib.name format
     * @param point = Profile point to check
     * @return True if this is a valid profile point
     */
    bool isProfilePointValid(const std::string& type, const std::string& point);

private:
    friend int ::main(int argc, char** argv);

    void notFound(const std::string& baseName, const std::string& type, const std::string& errorMsg);

    Factory(const std::string& searchPaths);
    ~Factory();

    Factory();                      // Don't Implement
    Factory(Factory const&);        // Don't Implement
    void operator=(Factory const&); // Don't Implement

    static Factory* instance;

    // find library information for name
    bool findLibrary(const std::string& name, std::ostream& err_os = std::cerr);
    // handle low-level loading of name
    bool loadLibrary(const std::string& name, std::ostream& err_os = std::cerr);

    std::set<std::string> loaded_libraries;

    std::string searchPaths;

    ElemLoader* loader;
    std::string loadingComponentType;

    std::pair<std::string, std::string> parseLoadName(const std::string& wholename);

    std::recursive_mutex factoryMutex;

protected:
    Output& out;
};

} // namespace SST

#endif // SST_CORE_FACTORY_H
