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

#ifndef SST_CORE_CONFIGSHARED_H
#define SST_CORE_CONFIGSHARED_H

#include "sst/core/configBase.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <iostream>


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
public:
    virtual ~ConfigShared() {}

    /**
       ConfigShared constructor that it meant to be used when needing
       a stand only ConfigShared (i.e. for the bootstrap wrappers for
       sst and sst-info
    */
    ConfigShared(bool suppress_print, bool suppress_sdl, bool include_libpath, bool include_env, bool include_verbose);

protected:
    void addLibraryPathOptions();
    void addEnvironmentOptions();
    void addVerboseOptions(bool sdl_avail);

    /**
       ConfigShared constructor for child classes
     */
    ConfigShared(bool suppress_print, bool suppress_sdl);

    /**
       Default constructor used for serialization.  At this point,
       my_rank is no longer needed, so just initialize to 0,0.
     */
    ConfigShared() : ConfigBase() {}

    // Variables that will need to be serialized by child class since
    // this class does not serialize itself due to being used in the
    // bootwrapper executables.

    //// Libpath options
    std::string libpath_    = "";
    std::string addlibpath_ = "";

    //// Environment Options
    bool print_env_     = false;
    bool no_env_config_ = false;

    //// Verbose Option
    int verbose_ = 0;

private:
    //// Libpath options

    // lib path
    int setLibPath(const std::string& arg)
    {
        libpath_ = arg;
        return 0;
    }

    // add to lib path
    int setAddLibPath(const std::string& arg)
    {
        if ( addlibpath_.length() > 0 ) addlibpath_ += std::string(":");
        addlibpath_ += arg;
        return 0;
    }

    //// Environment Options

    int enablePrintEnv(const std::string& UNUSED(arg))
    {
        printf("enablePrintEnv()\n");
        print_env_ = true;
        return 0;
    }

    int disableEnvConfig(const std::string& UNUSED(arg))
    {
        printf("disableEnvConfig()\n");
        no_env_config_ = true;
        return 0;
    }

    //// Verbose Option

    int setVerbosity(const std::string& arg)
    {
        if ( arg == "" ) {
            verbose_++;
            return 0;
        }
        try {
            unsigned long val = stoul(arg);
            verbose_          = val;
            return 0;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --verbose\n", arg.c_str());
            return -1;
        }
    }


public:
    /**
       Controls whether the environment variables that SST sees are
       printed out
    */
    bool print_env() const { return print_env_; }

    bool no_env_config() const { return no_env_config_; }

    int verbose() const { return verbose_; }

    std::string libpath() const { return libpath_; }
    std::string addLibPath() const { return addlibpath_; }

    std::string getLibPath(void) const;
};

} // namespace SST

#endif // SST_CORE_CONFIGBASE_H
