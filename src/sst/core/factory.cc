// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/factory.h"

#include "sst/core/component.h"
#include "sst/core/elemLoader.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/linkMap.h"
#include "sst/core/model/element_python.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/params.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

#include <fstream>
#include <ostream>
#include <set>
#include <stdio.h>
#include <tuple>

// Statistic Output Objects
#include "sst/core/statapi/statoutputcsv.h"
#include "sst/core/statapi/statoutputjson.h"
#include "sst/core/statapi/statoutputtxt.h"
#ifdef HAVE_HDF5
#include "sst/core/statapi/statoutputhdf5.h"
#endif
#include "sst/core/statapi/stataccumulator.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/stathistogram.h"
#include "sst/core/statapi/statnull.h"
#include "sst/core/statapi/statuniquecount.h"

using namespace SST::Statistics;

namespace SST {

Factory* Factory::instance = nullptr;
Output   Factory::out;

Factory*
Factory::createFactory(const std::string& searchPaths)
{
    if ( instance ) out.fatal(CALL_INFO, 1, "Already initialized a factory.\n");
    instance = new Factory(searchPaths);
    out      = Output::getDefaultObject();

    return instance;
}

Factory::Factory(const std::string& searchPaths) :
    searchPaths(searchPaths)
{
    loader = new ElemLoader(searchPaths);
    loaded_libraries.insert("sst");
}

Factory::~Factory()
{
    delete loader;
}

void
Factory::updateSearchPaths(const std::string& paths)
{
    searchPaths = paths;
    loader->updateSearchPaths(paths);
}

static bool
checkPort(const std::string& def, const std::string& offered)
{
    const char* x = def.c_str();
    const char* y = offered.c_str();

    /* Special case.  Name of '*' matches everything */
    if ( *x == '*' && *(x + 1) == '\0' ) return true;

    do {
        if ( *x == '%' && (*(x + 1) == '(' || *(x + 1) == 'd') ) {
            // We have a %d or %(var)d to eat
            x++;
            if ( *x == '(' ) {
                while ( *x && (*x != ')') )
                    x++;
                x++; /* *x should now == 'd' */
            }
            if ( *x != 'd' ) /* Malformed string.  Fail all the things */
                return false;
            x++; /* Finish eating the variable */
            /* Now, eat the corresponding digits of y */
            while ( *y && isdigit(*y) )
                y++;
        }
        if ( *x != *y ) return false;
        if ( *x == '\0' ) return true;
        x++;
        y++;
    } while ( *x && *y );
    if ( *x != *y ) return false; // aka, both nullptr
    return true;
}

bool
Factory::isPortNameValid(const std::string& type, const std::string& port_name)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);
    // ensure library is already loaded...
    if ( loaded_libraries.find(elemlib) == loaded_libraries.end() ) {
        findLibrary(elemlib);
    }

    const std::vector<std::string>* portNames = nullptr;

    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto*             lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    std::stringstream err;
    if ( lib ) {
        auto* compInfo = lib->getInfo(elem);
        if ( compInfo ) {
            portNames = &compInfo->getPortnames();
        }
    }
    if ( nullptr == portNames ) {
        auto* lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
        if ( lib ) {
            auto* subcompInfo = lib->getInfo(elem);
            if ( subcompInfo ) {
                portNames = &subcompInfo->getPortnames();
            }
            else {
                // this is going to fail
                err << "Valid SubComponents: ";
                for ( auto& pair : lib->getMap() ) {
                    err << pair.first << "\n";
                }
            }
        }
    }

    std::string tmp = elemlib + "." + elem;

    if ( portNames == nullptr ) {
        err << "Valid Components: ";
        for ( auto& pair : lib->getMap() ) {
            err << pair.first << "\n";
        }
        std::cerr << err.str() << std::endl;
        out.fatal(CALL_INFO, 1, "can't find requested component or subcomponent '%s'\n ", tmp.c_str());
        return false;
    }

    for ( auto p : *portNames ) {
        if ( checkPort(p, port_name) ) return true;
    }
    return false;
}

const std::vector<std::string>&
Factory::getParamNames(const std::string& type)
{
    // This is only needed so we can return something in the error
    // case.  It is never used.
    static std::vector<std::string> empty_keyset;

    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);
    // ensure library is already loaded...
    if ( loaded_libraries.find(elemlib) == loaded_libraries.end() ) {
        findLibrary(elemlib);
    }

    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto* lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if ( lib ) {
        auto* compInfo = lib->getInfo(elem);
        if ( compInfo ) return compInfo->getParamNames();
    }
    else {
        auto* lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
        if ( lib ) {
            auto* compInfo = lib->getInfo(elem);
            if ( compInfo ) return compInfo->getParamNames();
        }
        else {
            auto* lib = ELI::InfoDatabase::getLibrary<Module>(elemlib);
            if ( lib ) {
                auto* compInfo = lib->getInfo(elem);
                if ( compInfo ) return compInfo->getParamNames();
            }
        }
    }
    // If we made it to here we didn't find an element that has params
    // with the given name
    out.fatal(CALL_INFO, 1, "can't find requested element %s.\n ", type.c_str());
    return empty_keyset;
}

Component*
Factory::CreateComponent(ComponentId_t id, const std::string& type, Params& params)
{
    std::string elemlib, elem;

    std::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    std::stringstream sstr;
    requireLibrary(elemlib, sstr);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);
    // Check to see if library is loaded into new
    // ElementLibraryDatabase

    auto* lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if ( lib ) {
        auto* compInfo = lib->getInfo(elem);
        if ( compInfo ) {
            auto* compLib = Component::getBuilderLibrary(elemlib);
            if ( compLib ) {
                auto* fact = compLib->getBuilder(elem);
                if ( fact ) {
                    loadingComponentType = type;
                    params.pushAllowedKeys(compInfo->getParamNames());
                    Component* ret = fact->create(id, params);
                    params.popAllowedKeys();
                    loadingComponentType = "";
                    return ret;
                }
            }
        }
    }
    // If we make it to here, component not found
    out.fatal(CALL_INFO, 1, "can't find requested component '%s'\n%s\n", type.c_str(), sstr.str().c_str());
    return nullptr;
}

bool
Factory::DoesSubComponentSlotExist(const std::string& type, const std::string& slotName)
{
    std::string compTypeToLoad = type;

    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(compTypeToLoad);

    // ensure library is already loaded...
    std::stringstream error_os;
    requireLibrary(elemlib, error_os);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    // Check to see if library is loaded into new ElementLibraryDatabase
    auto* compLib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if ( compLib ) {
        auto* info = compLib->getInfo(elem);
        if ( info ) {
            for ( auto& item : info->getSubComponentSlots() ) {
                if ( item.name == slotName ) return true;
            }
            return false;
        }
    }

    auto* subLib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if ( subLib ) {
        auto* info = subLib->getInfo(elem);
        if ( info ) {
            for ( auto& item : info->getSubComponentSlots() ) {
                if ( item.name == slotName ) return true;
            }
            return false;
        }
    }

    // If we get to here, element doesn't exist
    out.fatal(
        CALL_INFO, 1, "can't find requested component/subcomponent '%s'\n%s\n", type.c_str(), error_os.str().c_str());
    return false;
}

uint8_t
Factory::GetStatisticValidityAndEnableLevel(const std::string& comp_type, const std::string& statistic_name)
{
    // returns 255 if invalid, else enable level
    // Allows single linear search of ELI for the statistic
    std::string comp_type_to_load = comp_type.empty() ? loadingComponentType : comp_type;

    // Look up Element
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(comp_type_to_load);
    std::stringstream error_os;
    requireLibrary(elemlib, error_os);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    // Check if elemlib is a component and if so, get ELI
    auto* comp_lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if ( comp_lib ) {
        auto* info = comp_lib->getInfo(elem);
        if ( info ) {
            auto stats = info->getValidStats();
            for ( auto stat : stats ) {
                if ( stat.name == statistic_name ) {
                    return stat.enable_level;
                }
            }
            return 255;
        }
    }

    // Check if elemlib is a subcomponent and if so, get ELI
    auto* sub_lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if ( sub_lib ) {
        auto* info = sub_lib->getInfo(elem);
        if ( info ) {
            auto stats = info->getValidStats();
            for ( auto stat : stats ) {
                if ( stat.name == statistic_name ) {
                    return stat.enable_level;
                }
            }
            return 255;
        }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, 1, "can't find requested component '%s'\n%s\n", comp_type.c_str(), error_os.str().c_str());
    return 255;
}

bool
Factory::DoesComponentInfoStatisticNameExist(const std::string& comp_type, const std::string& statistic_name)
{
    std::string comp_type_to_load = comp_type;
    if ( true == comp_type.empty() ) {
        comp_type_to_load = loadingComponentType;
    }

    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(comp_type_to_load);

    // ensure library is already loaded...
    std::stringstream error_os;
    requireLibrary(elemlib, error_os);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);

    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto* comp_lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if ( comp_lib ) {
        auto* info = comp_lib->getInfo(elem);
        if ( info ) {
            auto stats = info->getValidStats();
            for ( auto stat : stats ) {
                if ( stat.name == statistic_name ) return true;
            }
            return false;
        }
    }

    auto* sub_lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if ( sub_lib ) {
        auto* info = sub_lib->getInfo(elem);
        if ( info ) {
            auto stats = info->getValidStats();
            for ( auto stat : stats ) {
                if ( stat.name == statistic_name ) return true;
            }
            return false;
        }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, 1, "can't find requested component '%s'\n%s\n", comp_type.c_str(), error_os.str().c_str());
    return 0;
}

std::string
Factory::GetComponentInfoStatisticUnits(const std::string& type, const std::string& statistic_name)
{
    std::string comp_type_to_load = type;
    if ( true == type.empty() ) {
        comp_type_to_load = loadingComponentType;
    }

    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(comp_type_to_load);

    // ensure library is already loaded...
    std::stringstream error_os;
    if ( loaded_libraries.find(elemlib) == loaded_libraries.end() ) {
        findLibrary(elemlib, error_os);
    }

    auto* comp_lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    if ( comp_lib ) {
        auto* info = comp_lib->getInfo(elem);
        if ( info ) {
            auto stats = info->getValidStats();
            for ( auto stat : stats ) {
                if ( stat.name == statistic_name ) {
                    return stat.units;
                }
            }
        }
        return nullptr;
    }

    auto* sub_lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if ( sub_lib ) {
        auto* info = sub_lib->getInfo(elem);
        if ( info ) {
            auto stats = info->getValidStats();
            for ( auto stat : stats ) {
                if ( stat.name == statistic_name ) {
                    return stat.units;
                }
            }
        }
        return nullptr;
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, 1, "can't find requested component '%s'\n%s\n", type.c_str(), error_os.str().c_str());
    return nullptr;
}

bool
Factory::isProfilePointValid(const std::string& type, const std::string& point)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);
    // ensure library is already loaded...
    if ( loaded_libraries.find(elemlib) == loaded_libraries.end() ) {
        findLibrary(elemlib);
    }

    const std::vector<ElementInfoProfilePoint>* pointNames = nullptr;

    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto*             lib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    std::stringstream err;
    if ( lib ) {
        auto* compInfo = lib->getInfo(elem);
        if ( compInfo ) {
            pointNames = &compInfo->getProfilePoints();
        }
    }
    if ( nullptr == pointNames ) {
        auto* lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
        if ( lib ) {
            auto* subcompInfo = lib->getInfo(elem);
            if ( subcompInfo ) {
                pointNames = &subcompInfo->getProfilePoints();
            }
        }
    }

    std::string tmp = elemlib + "." + elem;

    if ( pointNames == nullptr ) {
        out.fatal(CALL_INFO, 1,
            "can't find requested component or subcomponent '%s' when searching for profile point\n ", tmp.c_str());
        return false;
    }

    for ( auto p : *pointNames ) {
        if ( p.name == point ) return true;
    }
    return false;
}


bool
Factory::doesSubComponentExist(const std::string& type)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    requireLibrary(elemlib);

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);
    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto*                                 lib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    if ( lib ) {
        auto* info = lib->getInfo(elem);
        if ( info ) return true;
    }

    // If we get to here, element doesn't exist
    return false;
}

void
Factory::RequireEvent(const std::string& eventname)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(eventname);

    // All we really need to do is make sure the library is loaded.
    // We no longer have events in the ELI
    requireLibrary(elemlib);
}

Partition::SSTPartitioner*
Factory::CreatePartitioner(const std::string& name, RankInfo total_ranks, RankInfo my_rank, int verbosity)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(name);

    // ensure library is already loaded...
    std::stringstream error_os;
    requireLibrary(elemlib, error_os);

    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto* lib = ELI::InfoDatabase::getLibrary<Partition::SSTPartitioner>(elemlib);
    if ( lib ) {
        auto* info = lib->getInfo(elem);
        if ( info ) {
            auto* builderLib = Partition::SSTPartitioner::getBuilderLibrary(elemlib);
            if ( builderLib ) {
                auto* fact = builderLib->getBuilder(elem);
                if ( fact ) {
                    return fact->create(total_ranks, my_rank, verbosity);
                }
            }
        }
    }

    // If we get to here, element doesn't exist
    out.fatal(CALL_INFO, 1,
        "Error: Unable to find requested partitioner '%s', check --help for information on partitioners.\n%s\n",
        name.c_str(), error_os.str().c_str());
    return nullptr;
}

// genPythonModuleFunction
SSTElementPythonModule*
Factory::getPythonModule(const std::string& name)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(name);

    // Check to see if library is loaded into new
    // ElementLibraryDatabase
    auto* lib = ELI::InfoDatabase::getLibrary<SSTElementPythonModule>(elemlib);
    if ( lib ) {
        auto* info = lib->getInfo(elem);
        if ( info ) {
            auto* builderLib = SSTElementPythonModule::getBuilderLibrary(elemlib);
            if ( builderLib ) {
                auto* fact = builderLib->getBuilder(elemlib);
                if ( fact ) {
                    auto* ret = fact->create(elemlib);
                    return ret;
                }
            }
        }
    }

    return nullptr;
}

bool
Factory::hasLibrary(const std::string& elemlib, std::ostream& error_os)
{
    return findLibrary(elemlib, error_os);
}

void
Factory::requireLibrary(const std::string& elemlib)
{
    if ( elemlib == "sst" ) return;

    // ignore error messages here
    std::ofstream ofs("/dev/null", std::ofstream::out | std::ofstream::app);
    requireLibrary(elemlib, ofs);
}

void
Factory::requireLibrary(const std::string& elemlib, std::ostream& error_os)
{
    if ( elemlib == "sst" ) return;

    // ignore error messages here
    // std::ofstream ofs("/dev/null", std::ofstream::out | std::ofstream::app);
    findLibrary(elemlib, error_os);
}

void
Factory::getLoadedLibraryNames(std::set<std::string>& lib_names)
{

    for ( auto& lib : loaded_libraries ) {
        lib_names.insert(lib);
    }
}

void
Factory::loadUnloadedLibraries(const std::set<std::string>& lib_names)
{
    for ( std::set<std::string>::const_iterator i = lib_names.begin(); i != lib_names.end(); ++i ) {
        findLibrary(*i);
    }
}

const std::string&
Factory::getSearchPaths()
{
    return searchPaths;
}

bool
Factory::findLibrary(const std::string& elemlib, std::ostream& err_os)
{

    std::lock_guard<std::recursive_mutex> lock(factoryMutex);
    if ( loaded_libraries.count(elemlib) == 1 ) return true;

    return loadLibrary(elemlib, err_os);
}

std::pair<std::string, std::string>
Factory::parseLoadName(const std::string& wholename)
{
    std::size_t found = wholename.find_first_of(".");
    if ( found == std::string::npos ) {
        if ( wholename.empty() ) {
            out.output(CALL_INFO, "Warning: got empty element library. "
                                  "You might have a missing parameter that causes a default empty string.");
        }
        return make_pair(wholename, wholename);
    }
    else {
        std::string eli(wholename, 0, found);
        std::string el(wholename, (size_t)(found + 1));
        return make_pair(eli, el);
    }
}

void
Factory::notFound(const std::string& baseName, const std::string& type, const std::string& error_msg)
{
    out.fatal(CALL_INFO, 1, "can't find requested element library '%s' with element type '%s'\n%s\n", baseName.c_str(),
        type.c_str(), error_msg.c_str());
}

bool
Factory::loadLibrary(const std::string& name, std::ostream& err_os)
{
    loader->loadLibrary(name, err_os);

    if ( !ELI::LoadedLibraries::isLoaded(name) ) {
        return false;
    }

    // The library was loaded, put it in loadedlibraries
    loaded_libraries.insert(name);
    return true;
}

SSTModelDescription*
Factory::createModelDescription(
    const std::string& type, const std::string& input_file, int verbose, Config& cfg, double start_time)
{
    std::string elemlib, elem;
    std::tie(elemlib, elem) = parseLoadName(type);

    // Model definitions have to be in the built-in sst library (which
    // is why this function can be static)
    if ( elemlib != "sst" ) return nullptr;

    std::stringstream err_os;

    auto* lib = ELI::InfoDatabase::getLibrary<SSTModelDescription>(elemlib);
    if ( lib ) {
        auto* info = lib->getInfo(elem);
        if ( info ) {
            auto* builderLib = SSTModelDescription::getBuilderLibrary(elemlib);
            if ( builderLib ) {
                auto* fact = builderLib->getBuilder(elem);
                if ( fact ) {
                    SSTModelDescription* ret = fact->create(input_file, verbose, &cfg, start_time);
                    return ret;
                }
            }
        }
    }
    return nullptr;
}


} // namespace SST
