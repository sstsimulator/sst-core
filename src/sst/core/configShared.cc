// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/configShared.h"

#include "sst/core/env/envquery.h"

#include <functional>

using namespace std;

namespace SST {

ConfigShared::ConfigShared(bool suppress_print, bool include_libpath, bool include_env, bool include_verbose) :
    ConfigBase(suppress_print)
{
    if ( include_libpath ) addLibraryPathOptions();
    if ( include_env ) addEnvironmentOptions();
    if ( include_verbose ) addVerboseOptions(false);
}


ConfigShared::ConfigShared(bool suppress_print, std::vector<AnnotationInfo> annotations) :
    ConfigBase(suppress_print, annotations)
{}

void
ConfigShared::addLibraryPathOptions()
{
    using namespace std::placeholders;
    // Add the options
    DEF_ARG(
        "lib-path", 0, "LIBPATH", "Component library path (overwrites default)",
        std::bind(&ConfigShared::setLibPath, this, _1), false);
    DEF_ARG(
        "add-lib-path", 0, "LIBPATH", "Component library path (appends to main path)",
        std::bind(&ConfigShared::setAddLibPath, this, _1), false);
}

void
ConfigShared::addEnvironmentOptions()
{
    using namespace std::placeholders;
    // Add the options
    DEF_FLAG(
        "print-env", 0, "Print environment variables SST will see", std::bind(&ConfigShared::enablePrintEnv, this, _1));
    DEF_FLAG(
        "no-env-config", 0, "Disable SST environment configuration",
        std::bind(&ConfigShared::disableEnvConfig, this, _1));
}

void
ConfigShared::addVerboseOptions(bool sdl_avail)
{
    using namespace std::placeholders;
    DEF_ARG_OPTVAL(
        "verbose", 'v', "level",
        "Verbosity level to determine what information about core runtime is printed.  If no argument is specified, it "
        "will simply increment the verbosity level.",
        std::bind(&ConfigShared::setVerbosity, this, _1), sdl_avail);
}


/**
   Get the library path for loading element libraries.  There are
   three places that paths can be specified and they are included in
   the path in the following order:

   1 - command line arguments set using --lib-path and --add-lib-path
   2 - paths set in environment variable SST_LIB_PATH
   3 - paths set in sstsimulator.conf file
   4 - (only for bootsst, Factory will not search this) LD_LIBRARY_PATH environment variable

 */
std::string
ConfigShared::getLibPath(void) const
{
    std::string fullLibPath;

    // This creates the following search order:
    //
    // 1 - command line
    // 2 - SST_LIB_PATH environment variable
    // 3 - Path from configuration file
    // 4 - User set LD_LIBRARY_PATH (only for final LD_LIBRARY_PATH)

    // The --lib-path option overwrites everything that comes before
    // (SST_LIB_PATH env variable and paths in sstsimulator.conf).
    if ( "" == libpath_ ) {

        // If the SST_LIB_PATH variable is set, put it into the final
        // path
        char* envpath = getenv("SST_LIB_PATH");
        if ( nullptr != envpath ) {
            fullLibPath.append(envpath);
            // delete envpath;
        }

        // Get configuration options from the user config
        std::vector<std::string>                          configPaths;
        SST::Core::Environment::EnvironmentConfiguration* envConfig =
            SST::Core::Environment::getSSTEnvironmentConfiguration(configPaths);

        std::set<std::string> configGroups = envConfig->getGroupNames();

        // iterate over groups of settings
        for ( auto groupItr = configGroups.begin(); groupItr != configGroups.end(); groupItr++ ) {
            SST::Core::Environment::EnvironmentConfigGroup* currentGroup = envConfig->getGroupByName(*groupItr);
            std::set<std::string>                           groupKeys    = currentGroup->getKeys();

            // find which keys have a LIBDIR at the END of the key we
            // recognize these may house elements
            for ( auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++ ) {
                const std::string& key   = *keyItr;
                const std::string& value = currentGroup->getValue(key);

                if ( key.size() >= 6 ) {
                    if ( "LIBDIR" == key.substr(key.size() - 6) ) {
                        // See if there is a value, if not, skip it
                        if ( value.size() > 0 ) {
                            // If this is the first one, we don't need
                            // a colon
                            if ( fullLibPath.size() > 0 ) fullLibPath.append(":");
                            fullLibPath.append(value);
                        }
                    }
                }
            }
        }

        // Clean up and delete the configuration we just loaded up
        delete envConfig;
    }
    else {
        // --lib-path was set, so just use it as the base
        fullLibPath = libpath_;
    }

    // NOw see if we need to prepend any paths from --add-lib-path
    if ( !addlibpath_.empty() ) {
        // fullLibPath.append(":");
        // fullLibPath.append(addlibpath_);
        fullLibPath.insert(0, ":");
        fullLibPath.insert(0, addlibpath_);
    }

    // if ( verbose_ ) {
    //     std::cout << "SST-Core: Configuration Library Path will read from: " << fullLibPath << std::endl;
    // }

    return fullLibPath;
}

} // namespace SST
