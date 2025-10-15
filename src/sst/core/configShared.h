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

#ifndef SST_CORE_CONFIGSHARED_H
#define SST_CORE_CONFIGSHARED_H

#include "sst/core/configBase.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>


namespace SST {

/**
 * Class to contain SST Simulation Configuration variables.
 *
 * NOTE: This class needs to be serialized for the sst.x executable,
 * but not for the sst (wrapper compiled from the boot*.{h,cc} files)
 * executable.  To avoid having to compile all the serialization code
 * into the bootstrap executable, the Config class is serialized in
 * serialization/serialize_config.h.
 */
class ConfigShared : public ConfigBase
{
    friend int ::main(int argc, char** argv);

public:
    virtual ~ConfigShared() {}

    /**
       ConfigShared constructor that it meant to be used when needing
       a stand alone ConfigShared (i.e. for the bootstrap wrappers for
       sst and sst-info
    */
    ConfigShared(bool suppress_print, bool include_libpath, bool include_env, bool include_verbose);

protected:
    void addLibraryPathOptions();
    void addEnvironmentOptions();
    void addVerboseOptions(bool sdl_avail);

    ConfigShared() = default;


private:
    //// Libpath options


    // lib path
    SST_CONFIG_DECLARE_OPTION(std::string, libpath, "", &StandardConfigParsers::from_string<std::string>);

    SST_CONFIG_DECLARE_OPTION(std::string, addLibPath, "",
        std::bind(&StandardConfigParsers::append_string, ":", "", std::placeholders::_1, std::placeholders::_2));


    //// Environment Options

    SST_CONFIG_DECLARE_OPTION(bool, print_env, false, &StandardConfigParsers::flag_set_true);

    SST_CONFIG_DECLARE_OPTION(bool, no_env_config, false, &StandardConfigParsers::flag_set_true);


    //// Verbose Option
    static int parse_verbosity(int32_t& val, std::string arg)
    {
        if ( arg == "" ) {
            val++;
            return 0;
        }
        try {
            unsigned long value = stoul(arg);
            val                 = value;
            return 0;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --verbose\n", arg.c_str());
            return -1;
        }
    }

    SST_CONFIG_DECLARE_OPTION(int32_t, verbose, 0, &ConfigShared::parse_verbosity);


public:

    std::string getLibPath() const;
};

} // namespace SST

#endif // SST_CORE_CONFIGBASE_H
