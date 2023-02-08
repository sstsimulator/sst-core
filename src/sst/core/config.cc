// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/config.h"

#include "sst/core/build_info.h"
#include "sst/core/env/envquery.h"
#include "sst/core/output.h"
#include "sst/core/part/sstpart.h"
#include "sst/core/warnmacros.h"

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include <cstdlib>
#include <errno.h>
#include <getopt.h>
#include <iostream>
#include <locale>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef E_OK
#define E_OK 0
#endif

using namespace std;

namespace SST {

// Reserved name to indicate section header in program options
static const char* SECTION_HEADER = "XSECTION_HEADERX";

//// Helper class for setting options

class ConfigHelper
{
private:
    Config& cfg;

public:
    typedef bool (ConfigHelper::*flagFunction)(void);
    typedef bool (ConfigHelper::*argFunction)(const std::string& arg);

    // Used to determine what return code to use
    bool clean_exit = false;

    ConfigHelper(Config& cfg) : cfg(cfg) {}

    // Print usage
    bool printHelp()
    {
        clean_exit = true;
        return usage();
    }

    // Prints the SST version
    bool printVersion()
    {
        printf("SST-Core Version (" PACKAGE_VERSION);
        if ( strcmp(SSTCORE_GIT_HEADSHA, PACKAGE_VERSION) ) {
            printf(", git branch : " SSTCORE_GIT_BRANCH);
            printf(", SHA: " SSTCORE_GIT_HEADSHA);
        }
        printf(")\n");

        clean_exit = true;
        return false; /* Should not continue */
    }

    // Verbosity
    bool incrVerbose()
    {
        cfg.verbose_++;
        return true;
    }

    bool setVerbosity(const std::string& arg)
    {
        try {
            unsigned long val = stoul(arg);
            cfg.verbose_      = val;
            return true;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --verbose\n", arg.c_str());
            return false;
        }
    }

    // num_threads
    bool setNumThreads(const std::string& arg)
    {
        try {
            unsigned long val      = stoul(arg);
            cfg.world_size_.thread = val;
            return true;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --num-threads\n", arg.c_str());
            return false;
        }
    }

    // SDL file
    bool setConfigFile(const std::string& arg)
    {
        cfg.configFile_ = arg;
        return true;
    }

    // model_options
    bool setModelOptions(const std::string& arg)
    {
        if ( cfg.model_options_.empty() )
            cfg.model_options_ = arg;
        else
            cfg.model_options_ += std::string(" \"") + arg + std::string("\"");
        return true;
    }

    // print timing
    bool enablePrintTiming()
    {
        cfg.print_timing_ = true;
        return true;
    }

    bool setPrintTiming(const std::string& arg)
    {
        bool success      = false;
        cfg.print_timing_ = parseBoolean(arg, success, "enable-print-timing");
        return success;
    }

    // stop-at
    bool setStopAt(const std::string& arg)
    {
        cfg.stop_at_ = arg;
        return true;
    }

    // stopAtCycle (deprecated)
    bool setStopAtCycle(const std::string& arg)
    {
        // Uncomment this once all test files have been updated
        // fprintf(stderr, "Program option 'stopAtCycle' has been deprecated.  Please use 'stop-at' instead");
        return setStopAt(arg);
    }

    // exit after
    bool setExitAfter(const std::string& arg)
    {
        /* TODO: Error checking */
        errno = 0;

        static const char* templates[] = { "%H:%M:%S", "%M:%S", "%S", "%Hh", "%Mm", "%Ss" };
        const size_t       n_templ     = sizeof(templates) / sizeof(templates[0]);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        struct tm res = {}; /* This warns on GCC 4.8 due to a bug in GCC */
#pragma GCC diagnostic pop
        char* p;

        for ( size_t i = 0; i < n_templ; i++ ) {
            memset(&res, '\0', sizeof(res));
            p = strptime(arg.c_str(), templates[i], &res);
            fprintf(
                stderr, "**** [%s]  p = %p ; *p = '%c', %u:%u:%u\n", templates[i], p, (p) ? *p : '\0', res.tm_hour,
                res.tm_min, res.tm_sec);
            if ( p != nullptr && *p == '\0' ) {
                cfg.exit_after_ = res.tm_sec;
                cfg.exit_after_ += res.tm_min * 60;
                cfg.exit_after_ += res.tm_hour * 60 * 60;
                return true;
            }
        }

        fprintf(
            stderr,
            "Failed to parse stop time [%s]\n"
            "Valid formats are:\n",
            arg.c_str());
        for ( size_t i = 0; i < n_templ; i++ ) {
            fprintf(stderr, "\t%s\n", templates[i]);
        }

        return false;
    }

    // stop after (deprecated)
    bool setStopAfter(const std::string& arg)
    {
        // Uncomment this once all test files have been updated
        // fprintf(stderr, "Program option 'stopAfter' has been deprecated.  Please use 'exit-after' instead");
        return setExitAfter(arg);
    }


    // partitioner
    bool setPartitioner(const std::string& arg)
    {
        cfg.partitioner_ = arg;
        if ( cfg.partitioner_.find('.') == cfg.partitioner_.npos ) { cfg.partitioner_ = "sst." + cfg.partitioner_; }
        return true;
    }

    // heart beat
    bool setHeartbeat(const std::string& arg)
    {
        /* TODO: Error checking */
        cfg.heartbeatPeriod_ = arg;
        return true;
    }

    // output directory
    bool setOutputDir(const std::string& arg)
    {
        cfg.output_directory_ = arg;
        return true;
    }

    // core output prefix
    bool setOutputPrefix(const std::string& arg)
    {
        cfg.output_core_prefix_ = arg;
        return true;
    }


    // Configuration output

    // output config python
    bool setWriteConfig(const std::string& arg)
    {
        cfg.output_config_graph_ = arg;
        return true;
    }

    // output config json
    bool setWriteJSON(const std::string& arg)
    {
        cfg.output_json_ = arg;
        return true;
    }

    // parallel output
#ifdef SST_CONFIG_HAVE_MPI
    bool enableParallelOutput()
    {
        // If this is only a one rank job, then we can ignore
        if ( cfg.world_size_.rank == 1 ) return true;

        cfg.parallel_output_  = true;
        // For parallel output, we always need to output the partition
        // info as well
        cfg.output_partition_ = true;
        return true;
    }

    bool enableParallelOutputArg(const std::string& arg)
    {
        // If this is only a one rank job, then we can ignore, except
        // that we will turn on output_partition
        if ( cfg.world_size_.rank == 1 ) return true;

        bool success = false;

        cfg.parallel_output_  = parseBoolean(arg, success, "parallel-output");
        // For parallel output, we always need to output the partition
        // info as well.  Also, if it was already set to true, don't
        // overwrite even if parallel_output was set to false.
        cfg.output_partition_ = cfg.output_partition_ | cfg.parallel_output_;
        return success;
    }
#endif


    // Graph output

    // graph output dot
    bool setWriteDot(const std::string& arg)
    {
        cfg.output_dot_ = arg;
        return true;
    }

    // dot verbosity
    bool setDotVerbosity(const std::string& arg)
    {
        try {
            unsigned long val  = stoul(arg);
            cfg.dot_verbosity_ = val;
            return true;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --dot-verbosity\n", arg.c_str());
            return false;
        }
    }

    // output partition
    bool setWritePartitionFile(const std::string& arg)
    {
        cfg.component_partition_file_ = arg;
        return true;
    }

    bool setWritePartition()
    {
        cfg.output_partition_ = true;
        return true;
    }

    // parallel load
#ifdef SST_CONFIG_HAVE_MPI
    bool enableParallelLoadMode(const std::string& arg)
    {
        // If this is only a one rank job, then we can ignore
        if ( cfg.world_size_.rank == 1 ) return true;

        std::string arg_lower(arg);
        std::locale loc;
        for ( auto& ch : arg_lower )
            ch = std::tolower(ch, loc);

        if ( arg_lower == "none" ) {
            cfg.parallel_load_ = false;
            return true;
        }
        else
            cfg.parallel_load_ = true;

        if ( arg_lower == "single" )
            cfg.parallel_load_mode_multi_ = false;
        else if ( arg_lower == "multi" )
            cfg.parallel_load_mode_multi_ = true;
        else {
            fprintf(
                stderr, "Invalid option '%s' passed to --parallel-load.  Valid options are NONE, SINGLE and MULTI.\n",
                arg.c_str());
            return false;
        }

        return true;
    }

    bool enableParallelLoad()
    {
        // If this is only a one rank job, then we can ignore
        if ( cfg.world_size_.rank == 1 ) return true;

        cfg.parallel_load_ = true;
        return true;
    }
#endif

    // Advanced options

    // timebase
    bool setTimebase(const std::string& arg)
    {
        // TODO: Error checking.  Need to wait until UnitAlgebra
        // switches to exceptions on errors instead of calling abort
        cfg.timeBase_ = arg;
        return true;
    }

    // timevortex
    bool setTimeVortex(const std::string& arg)
    {
        cfg.timeVortex_ = arg;
        return true;
    }

    // interthread links
    bool setInterThreadLinks()
    {
        cfg.interthread_links_ = true;
        return true;
    }

    bool setInterThreadLinksArg(const std::string& arg)
    {
        bool success           = false;
        cfg.interthread_links_ = parseBoolean(arg, success, "interthread-links");
        return success;
    }

#ifdef USE_MEMPOOL
    // cache align mempool allocations
    bool setCacheAlignMempools()
    {
        cfg.cache_align_mempools_ = true;
        return true;
    }

    bool setCacheAlignMempoolsArg(const std::string& arg)
    {
        bool success              = false;
        cfg.cache_align_mempools_ = parseBoolean(arg, success, "cache-align-mempools");
        return success;
    }
#endif

    // debug file
    bool setDebugFile(const std::string& arg)
    {
        cfg.debugFile_ = arg;
        return true;
    }

    // lib path
    bool setLibPath(const std::string& arg)
    {
        /* TODO: Error checking */
        cfg.libpath_ = arg;
        return true;
    }

    // add to lib path
    bool addLibPath(const std::string& arg)
    {
        /* TODO: Error checking */
        cfg.libpath_ += std::string(":") + arg;
        return true;
    }

    // Advanced options - profiling
    bool enableProfiling(const std::string& arg)
    {
        cfg.enabled_profiling_ = arg;
        return true;
    }

    bool setProfilingOutput(const std::string& arg)
    {
        cfg.profiling_output_ = arg;
        return true;
    }


    // Advanced options - debug

    // run mode
    bool setRunMode(const std::string& arg)
    {
        if ( !arg.compare("init") )
            cfg.runMode_ = Simulation::INIT;
        else if ( !arg.compare("run") )
            cfg.runMode_ = Simulation::RUN;
        else if ( !arg.compare("both") )
            cfg.runMode_ = Simulation::BOTH;
        else {
            fprintf(stderr, "Unknown option for --run-mode: %s\n", arg.c_str());
            cfg.runMode_ = Simulation::UNKNOWN;
        }

        return cfg.runMode_ != Simulation::UNKNOWN;
    }

    // dump undeleted events
#ifdef USE_MEMPOOL
    bool setWriteUndeleted(const std::string& arg)
    {
        cfg.event_dump_file_ = arg;
        return true;
    }
#endif

    // rank sequentional startup
    bool forceRankSeqStartup()
    {
        cfg.rank_seq_startup_ = true;
        return true;
    }


    // Advanced options - environment

    // print environment
    bool enablePrintEnv()
    {
        cfg.print_env_ = true;
        return true;
    }

    // Disable signal handlers
    bool disableSigHandlers()
    {
        cfg.enable_sig_handling_ = false;
        return true;
    }

    // Disable environment loader
    bool disableEnvConfig()
    {
        cfg.no_env_config_ = true;
        return true;
    }

    /**
       Special function for catching the magic string indicating a
       section header in the options array.  It will report that the
       value is unknown.
     */
    bool sectionHeaderCatch()
    {
        // This will only ever get called, if, for some reason, someone
        // tries to use the reserved section header name as an argument to
        // sst.  In that case, make it look the same as any other unknown
        // argument.
        fprintf(stderr, "sst: unrecognized option --%s\n", SECTION_HEADER);
        usage();
        return false;
    }

    /**
       Special function for catching the magic string indicating a
       section header in the options array.  It will report that the
       value is unknown.
     */
    bool sectionHeaderCatchArg(const std::string& UNUSED(arg))
    {
        // This will only ever get called, if, for some reason, someone
        // tries to use the reserved section header name as an argument to
        // sst.  In that case, make it look the same as any other unknown
        // argument.
        fprintf(stderr, "sst: unrecognized option --%s\n", SECTION_HEADER);
        usage();
        return false;
    }


    // Prints the usage information
    bool usage();

    // Function to uniformly parse boolean values for command line
    // arguments
    bool parseBoolean(const std::string& arg, bool& success, const std::string& option);
};


//// Config Class functions

void
Config::print()
{
    std::cout << "verbose = " << verbose_ << std::endl;
    std::cout << "num_threads = " << world_size_.thread << std::endl;
    std::cout << "num_ranks = " << world_size_.rank << std::endl;
    std::cout << "configFile = " << configFile_ << std::endl;
    std::cout << "model_options = " << model_options_ << std::endl;
    std::cout << "print_timing = " << print_timing_ << std::endl;
    std::cout << "stop_at = " << stop_at_ << std::endl;
    std::cout << "exit_after = " << exit_after_ << std::endl;
    std::cout << "partitioner = " << partitioner_ << std::endl;
    std::cout << "heartbeatPeriod = " << heartbeatPeriod_ << std::endl;
    std::cout << "output_directory = " << output_directory_ << std::endl;
    std::cout << "output_core_prefix = " << output_core_prefix_ << std::endl;
    std::cout << "output_config_graph = " << output_config_graph_ << std::endl;
    std::cout << "output_json = " << output_json_ << std::endl;
    std::cout << "parallel_output = " << parallel_output_ << std::endl;
    std::cout << "output_dot = " << output_dot_ << std::endl;
    std::cout << "dot_verbosity = " << dot_verbosity_ << std::endl;
    std::cout << "component_partition_file = " << component_partition_file_ << std::endl;
    std::cout << "output_partition = " << output_partition_ << std::endl;
    std::cout << "timeBase = " << timeBase_ << std::endl;
    std::cout << "parallel_load = " << parallel_load_ << std::endl;
    std::cout << "timeVortex = " << timeVortex_ << std::endl;
    std::cout << "interthread_links = " << interthread_links_ << std::endl;
#ifdef USE_MEMPOOL
    std::cout << "cache_align_mempools = " << cache_align_mempools_ << std::endl;
#endif
    std::cout << "debugFile = " << debugFile_ << std::endl;
    std::cout << "libpath = " << libpath_ << std::endl;
    std::cout << "addLlibPath = " << addLibPath_ << std::endl;
    std::cout << "enabled_profiling = " << enabled_profiling_ << std::endl;
    std::cout << "profiling_output = " << profiling_output_ << std::endl;
    std::cout << "runMode = " << runMode_ << std::endl;
#ifdef USE_MEMPOOL
    std::cout << "event_dump_file = " << event_dump_file_ << std::endl;
#endif
    std::cout << "rank_seq_startup_ " << rank_seq_startup_ << std::endl;
    std::cout << "print_env" << print_env_ << std::endl;
    std::cout << "enable_sig_handling = " << enable_sig_handling_ << std::endl;
    std::cout << "no_env_config = " << no_env_config_ << std::endl;
}


Config::Config(RankInfo rank_info) : Config()
{
    // Basic Options
    verbose_           = 0;
    world_size_.rank   = rank_info.rank;
    world_size_.thread = 1;
    configFile_        = "NONE";
    model_options_     = "";
    print_timing_      = false;
    stop_at_           = "0 ns";
    exit_after_        = 0;
    partitioner_       = "sst.linear";
    heartbeatPeriod_   = "";

    char* wd_buf = (char*)malloc(sizeof(char) * PATH_MAX);
    getcwd(wd_buf, PATH_MAX);

    output_directory_ = "";
    if ( nullptr != wd_buf ) {
        output_directory_.append(wd_buf);
        free(wd_buf);
    }

    output_core_prefix_ = "@x SST Core: ";

    // Configuration Output

    output_config_graph_ = "";
    output_json_         = "";
    parallel_output_     = false;

    // Graph output
    output_dot_               = "";
    dot_verbosity_            = 0;
    component_partition_file_ = "";
    output_partition_         = false;

    // Advance Options
    timeBase_                 = "1 ps";
    parallel_load_            = false;
    parallel_load_mode_multi_ = true;
    timeVortex_               = "sst.timevortex.priority_queue";
    interthread_links_        = false;
#ifdef USE_MEMPOOL
    cache_align_mempools_ = false;
#endif
    debugFile_  = "/dev/null";
    libpath_    = SST_INSTALL_PREFIX "/lib/sst";
    addLibPath_ = "";

    // Advance Options - Profiling
    enabled_profiling_ = "";
    profiling_output_  = "stdout";

    // Advanced Options - Debug
    runMode_ = Simulation::BOTH;
#ifdef __SST_DEBUG_EVENT_TRACKING__
    event_dump_file_ = "";
#endif
    rank_seq_startup_ = false;

    // Advanced Options - environment
    print_env_           = false;
    enable_sig_handling_ = true;
    no_env_config_       = false;
}


// Check for existence of config file
bool
Config::checkConfigFile()
{
    struct stat sb;
    char*       fqpath = realpath(configFile_.c_str(), nullptr);
    if ( nullptr == fqpath ) {
        fprintf(stderr, "Failed to canonicalize path [%s]:  %s\n", configFile_.c_str(), strerror(errno));
        return false;
    }
    configFile_ = fqpath;
    free(fqpath);
    if ( 0 != stat(configFile_.c_str(), &sb) ) {
        fprintf(stderr, "File [%s] cannot be found: %s\n", configFile_.c_str(), strerror(errno));
        return false;
    }
    if ( !S_ISREG(sb.st_mode) ) {
        fprintf(stderr, "File [%s] is not a regular file.\n", configFile_.c_str());
        return false;
    }

    if ( 0 != access(configFile_.c_str(), R_OK) ) {
        fprintf(stderr, "File [%s] is not readable.\n", configFile_.c_str());
        return false;
    }

    return true;
}

std::string
Config::getLibPath(void) const
{
    char* envpath = getenv("SST_LIB_PATH");

    // Get configuration options from the user config
    std::vector<std::string>                          overrideConfigPaths;
    SST::Core::Environment::EnvironmentConfiguration* envConfig =
        SST::Core::Environment::getSSTEnvironmentConfiguration(overrideConfigPaths);

    std::string           fullLibPath  = libpath_;
    std::set<std::string> configGroups = envConfig->getGroupNames();

    // iterate over groups of settings
    for ( auto groupItr = configGroups.begin(); groupItr != configGroups.end(); groupItr++ ) {
        SST::Core::Environment::EnvironmentConfigGroup* currentGroup = envConfig->getGroupByName(*groupItr);
        std::set<std::string>                           groupKeys    = currentGroup->getKeys();

        // find which keys have a LIBDIR at the END of the key
        // we recognize these may house elements
        for ( auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++ ) {
            const std::string& key   = *keyItr;
            const std::string& value = currentGroup->getValue(key);

            if ( key.size() > 6 ) {
                if ( "LIBDIR" == key.substr(key.size() - 6) ) {
                    fullLibPath.append(":");
                    fullLibPath.append(value);
                }
            }
        }
    }

    // Clean up and delete the configuration we just loaded up
    delete envConfig;

    if ( nullptr != envpath ) {
        fullLibPath.clear();
        fullLibPath.append(envpath);
    }

    if ( !addLibPath_.empty() ) {
        fullLibPath.append(":");
        fullLibPath.append(addLibPath_);
    }

    if ( verbose_ ) {
        std::cout << "SST-Core: Configuration Library Path will read from: " << fullLibPath << std::endl;
    }

    return fullLibPath;
}


// Struct to hold the program option information
struct sstLongOpts_s
{
    struct option              opt;
    const char*                argName;
    const char*                desc;
    ConfigHelper::flagFunction flagFunc;
    ConfigHelper::argFunction  argFunc;
    bool                       sdl_avail;
    // Unlike the other variables that hold the information about the
    // option, this is used to keep track of options that are set on
    // the command line.  We need this so we can ignore those when
    // setting options from the sdl file.
    mutable bool               set_cmdline;
};


// Macros to make defining options easier
// Nomenaclature is:

// FLAG - value is either true or false.  FLAG defaults to no arguments allowed
// ARG - value is a string.  ARG defaults to required argument
// OPTVAL - Takes an optional paramater

// longName - multicharacter name referenced using --
// shortName - single character name referenced using -
// text - help text
// func - function called if there is no value specified
// valFunct - function called if there is a value specified
// sdl_avail - determines whether option is avaialble to set in input files
#define DEF_FLAG_OPTVAL(longName, shortName, text, func, valFunc, sdl_avail)                           \
    {                                                                                                  \
        { longName, optional_argument, 0, shortName }, "[BOOL]", text, func, valFunc, sdl_avail, false \
    }

#define DEF_FLAG(longName, shortName, text, func)                                           \
    {                                                                                       \
        { longName, no_argument, 0, shortName }, nullptr, text, func, nullptr, false, false \
    }

#define DEF_ARG(longName, shortName, argName, text, valFunc, sdl_avail)                                  \
    {                                                                                                    \
        { longName, required_argument, 0, shortName }, argName, text, nullptr, valFunc, sdl_avail, false \
    }

//#define DEF_ARGOPT(longName, argName, text, func) DEF_ARGOPT_SHORT(longName, 0, argName, text, func)

#define DEF_ARG_OPTVAL(longName, shortName, argName, text, func, valFunc, sdl_avail)                          \
    {                                                                                                         \
        { longName, optional_argument, 0, shortName }, "[" argName "]", text, func, valFunc, sdl_avail, false \
    }


#define DEF_SECTION_HEADING(text)                                                                      \
    {                                                                                                  \
        { SECTION_HEADER, optional_argument, 0, 0 }, nullptr, text, &ConfigHelper::sectionHeaderCatch, \
            &ConfigHelper::sectionHeaderCatchArg, false, false                                         \
    }

static const struct sstLongOpts_s sstOptions[] = {
    /* Informational options */
    DEF_SECTION_HEADING("Informational Options"),
    DEF_FLAG("help", 'h', "Print help message", &ConfigHelper::printHelp),
    DEF_FLAG("version", 'V', "Print SST Release Version", &ConfigHelper::printVersion),

    /* Basic Options */
    DEF_SECTION_HEADING("Basic Options (most commonly used)"),
    DEF_ARG_OPTVAL(
        "verbose", 'v', "level", "Verbosity level to determine what information about core runtime is printed",
        &ConfigHelper::incrVerbose, &ConfigHelper::setVerbosity, true),
    DEF_ARG(
        "num-threads", 'n', "NUM", "Number of parallel threads to use per rank", &ConfigHelper::setNumThreads, true),
    DEF_ARG(
        "num_threads", 0, "NUM",
        "[Deprecated] Number of parallel threads to use per rank (deprecated, please use --num-threads or -n instead)",
        &ConfigHelper::setNumThreads, true),
    DEF_ARG(
        "sdl-file", 0, "FILE",
        "Specify SST Configuration file.  Note: this is most often done by just specifying the file without an option.",
        &ConfigHelper::setConfigFile, false),
    DEF_ARG(
        "model-options", 0, "STR",
        "Provide options to the python configuration script.  Additionally, any arguments provided after a final '-- ' "
        "will be "
        "appended to the model options (or used as the model options if --model-options was not specified).",
        &ConfigHelper::setModelOptions, false),
    DEF_FLAG_OPTVAL(
        "print-timing-info", 0, "Print SST timing information", &ConfigHelper::enablePrintTiming,
        &ConfigHelper::setPrintTiming, true),
    DEF_ARG("stop-at", 0, "TIME", "Set time at which simulation will end execution", &ConfigHelper::setStopAt, true),
    DEF_ARG(
        "stopAtCycle", 0, "TIME",
        "[DEPRECATED] Set time at which simulation will end execution (deprecated, please use --stop-at instead)",
        &ConfigHelper::setStopAtCycle, true),
    DEF_ARG(
        "exit-after", 0, "TIME",
        "Set the maximum wall time after which simulation will end execution.  Time is specified in hours, minutes and "
        "seconds, with the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (captital letters are the "
        "appropriate numbers for that value, lower case letters represent the units and are required for those "
        "formats).",
        &ConfigHelper::setExitAfter, true),
    DEF_ARG(
        "stopAfter", 0, "TIME",
        "[DEPRECATED] Set the maximum wall time after which simulation will exit (deprecatd, please use --exit-after "
        "instead",
        &ConfigHelper::setStopAfter, true),
    DEF_ARG(
        "partitioner", 0, "PARTITIONER", "Select the partitioner to be used. <lib.partitionerName>",
        &ConfigHelper::setPartitioner, true),
    DEF_ARG(
        "heartbeat-period", 0, "PERIOD",
        "Set time for heartbeats to be published (these are approximate timings, published by the core, to update on "
        "progress)",
        &ConfigHelper::setHeartbeat, true),
    DEF_ARG(
        "output-directory", 0, "DIR", "Directory into which all SST output files should reside",
        &ConfigHelper::setOutputDir, true),
    DEF_ARG(
        "output-prefix-core", 0, "STR", "set the SST::Output prefix for the core", &ConfigHelper::setOutputPrefix,
        true),

    /* Configuration Output */
    DEF_SECTION_HEADING(
        "Configuration Output Options (generates a file that can be used as input for reproducing a run)"),
    DEF_ARG(
        "output-config", 0, "FILE", "File to write SST configuration (in Python format)", &ConfigHelper::setWriteConfig,
        true),
    DEF_ARG(
        "output-json", 0, "FILE", "File to write SST configuration graph (in JSON format)", &ConfigHelper::setWriteJSON,
        true),
#ifdef SST_CONFIG_HAVE_MPI
    DEF_FLAG_OPTVAL(
        "parallel-output", 0,
        "Enable parallel output of configuration information.  This option is ignored for single rank jobs.  Must also "
        "specify an output type (--output-config "
        "and/or --output-json).  Note: this will also cause partition info to be output if set to true.",
        &ConfigHelper::enableParallelOutput, &ConfigHelper::enableParallelOutputArg, true),
#endif

    /* Configuration Output */
    DEF_SECTION_HEADING("Graph Output Options (for outputting graph information for visualization or inspection)"),
    DEF_ARG(
        "output-dot", 0, "FILE", "File to write SST configuration graph (in GraphViz format)",
        &ConfigHelper::setWriteDot, true),
    DEF_ARG(
        "dot-verbosity", 0, "INT", "Amount of detail to include in the dot graph output",
        &ConfigHelper::setDotVerbosity, true),
    DEF_ARG_OPTVAL(
        "output-partition", 0, "FILE",
        "File to write SST component partitioning information.  When used without an argument and in conjuction with "
        "--output-json or --output-config options, will cause paritition information to be added to graph output.",
        &ConfigHelper::setWritePartition, &ConfigHelper::setWritePartitionFile, true),

    /* Advanced Features */
    DEF_SECTION_HEADING("Advanced Options"),
    DEF_ARG(
        "timebase", 0, "TIMEBASE", "Set the base time step of the simulation (default: 1ps)",
        &ConfigHelper::setTimebase, true),
#ifdef SST_CONFIG_HAVE_MPI
    DEF_ARG_OPTVAL(
        "parallel-load", 0, "MODE",
        "Enable parallel loading of configuration. This option is ignored for single rank jobs.  Optional mode "
        "parameters are NONE, SINGLE and MULTI (default).  If NONE is specified, parallel-load is turned off. If "
        "SINGLE is specified, the same file will be passed to all MPI "
        "ranks.  If MULTI is specified, each MPI rank is required to have it's own file to load. Note, not all input "
        "formats support both types of file loading.",
        &ConfigHelper::enableParallelLoad, &ConfigHelper::enableParallelLoadMode, false),
#endif
    DEF_ARG(
        "timeVortex", 0, "MODULE", "Select TimeVortex implementation <lib.timevortex>", &ConfigHelper::setTimeVortex,
        true),
    DEF_FLAG_OPTVAL(
        "interthread-links", 0, "[EXPERIMENTAL] Set whether or not interthread links should be used",
        &ConfigHelper::setInterThreadLinks, &ConfigHelper::setInterThreadLinksArg, true),
#ifdef USE_MEMPOOL
    DEF_FLAG_OPTVAL(
        "cache-align-mempools", 0, "[EXPERIMENTAL] Set whether mempool allocations are cache aligned",
        &ConfigHelper::setCacheAlignMempools, &ConfigHelper::setCacheAlignMempoolsArg, true),
#endif
    DEF_ARG("debug-file", 0, "FILE", "File where debug output will go", &ConfigHelper::setDebugFile, true),
    DEF_ARG("lib-path", 0, "LIBPATH", "Component library path (overwrites default)", &ConfigHelper::setLibPath, true),
    DEF_ARG(
        "add-lib-path", 0, "LIBPATH", "Component library path (appends to main path)", &ConfigHelper::addLibPath, true),

    /* Advanced Features - Profiling */
    DEF_SECTION_HEADING("Advanced Options - Profiling (EXPERIMENTAL)"),
    DEF_ARG(
        "enable-profiling", 0, "POINTS",
        "Enables default profiling for the specified points.  Argument is a semicolon separated list specifying the "
        "points to enable.",
        &ConfigHelper::enableProfiling, true),
    DEF_ARG(
        "profiling-output", 0, "FILE", "Set output location for profiling data [stdout (default) or a filename]",
        &ConfigHelper::setProfilingOutput, true),

    /* Advanced Features - Debug */
    DEF_SECTION_HEADING("Advanced Options - Debug"),
    DEF_ARG("run-mode", 0, "MODE", "Set run mode [ init | run | both (default)]", &ConfigHelper::setRunMode, true),
#ifdef USE_MEMPOOL
    DEF_ARG(
        "output-undeleted-events", 0, "FILE",
        "file to write information about all undeleted events at the end of simulation (STDOUT and STDERR can be used "
        "to output to console)",
        &ConfigHelper::setWriteUndeleted, true),
#endif
    DEF_FLAG(
        "force-rank-seq-startup", 0,
        "Force startup phases of simulation to execute one rank at a time for debug purposes",
        &ConfigHelper::forceRankSeqStartup),

    /* Advanced Features - Environment Setup/Reporting */
    DEF_SECTION_HEADING("Advanced Options - Environment Setup/Reporting"),
    DEF_FLAG("print-env", 0, "Print environment variables SST will see", &ConfigHelper::enablePrintEnv),
    DEF_FLAG("disable-signal-handlers", 0, "Disable signal handlers", &ConfigHelper::disableSigHandlers),
    DEF_FLAG("no-env-config", 0, "Disable SST environment configuration", &ConfigHelper::disableEnvConfig),
    { { nullptr, 0, nullptr, 0 }, nullptr, nullptr, nullptr, nullptr, false, false }
};
static const size_t nLongOpts = (sizeof(sstOptions) / sizeof(sstLongOpts_s)) - 1;


bool
ConfigHelper::parseBoolean(const std::string& arg, bool& success, const std::string& option)
{
    success = true;

    std::string arg_lower(arg);
    std::locale loc;
    for ( auto& ch : arg_lower )
        ch = std::tolower(ch, loc);

    if ( arg_lower == "true" || arg_lower == "yes" || arg_lower == "1" || arg_lower == "on" )
        return true;
    else if ( arg_lower == "false" || arg_lower == "no" || arg_lower == "0" || arg_lower == "off" )
        return false;
    else {
        fprintf(
            stderr,
            "ERROR: Failed to parse \"%s\" as a bool for option \"%s\", "
            "please use true/false, yes/no or 1/0\n",
            arg.c_str(), option.c_str());
        exit(-1);
        return false;
    }
}


bool
ConfigHelper::usage()
{
#ifdef SST_CONFIG_HAVE_MPI
    int this_rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);
    if ( this_rank != 0 ) return false;
#endif

    /* Determine screen / description widths */
    uint32_t MAX_WIDTH = 80;

    struct winsize size;
    if ( ioctl(STDERR_FILENO, TIOCGWINSZ, &size) == 0 ) { MAX_WIDTH = size.ws_col; }

    if ( getenv("COLUMNS") ) {
        errno      = E_OK;
        uint32_t x = strtoul(getenv("COLUMNS"), nullptr, 0);
        if ( errno == E_OK ) MAX_WIDTH = x;
    }

    const char     sdl_indicator[] = "(S)";
    const uint32_t sdl_start       = 34;
    const uint32_t desc_start      = sdl_start + sizeof(sdl_indicator);
    const uint32_t desc_width      = MAX_WIDTH - desc_start;

    /* Print usage */
    fprintf(
        stderr, "Usage: sst [options] config-file\n"
                "  Arguments to options contained in [] are optional\n"
                "  Options available to be set in the sdl file (input configuration file) are denoted by (S)\n"
                "   - Options set on the command line take precedence over options set in the SDL file\n"
                "  Notes on flag options (options that take an optional BOOL value):\n"
                "   - BOOL values can be expressed as true/false, yes/no, on/off or 1/0\n"
                "   - Program default for flags is false\n"
                "   - BOOL values default to true if flag is specified but no value is supplied\n");

    for ( size_t i = 0; i < nLongOpts; i++ ) {
        if ( !strcmp(sstOptions[i].opt.name, SECTION_HEADER) ) {
            // Just a section heading
            fprintf(stderr, "\n%s\n", sstOptions[i].desc);
            continue;
        }

        uint32_t npos = 0;
        if ( sstOptions[i].opt.val ) { npos += fprintf(stderr, "-%c ", (char)sstOptions[i].opt.val); }
        else {
            npos += fprintf(stderr, "   ");
        }
        npos += fprintf(stderr, "--%s", sstOptions[i].opt.name);
        if ( sstOptions[i].opt.has_arg != no_argument ) { npos += fprintf(stderr, "=%s", sstOptions[i].argName); }
        // If we have already gone beyond the description start,
        // description starts on new line
        if ( npos >= sdl_start ) {
            fprintf(stderr, "\n");
            npos = 0;
        }

        // If this can be set in the sdl file, start description with
        // "(S)"
        while ( npos < sdl_start ) {
            npos += fprintf(stderr, " ");
        }
        if ( sstOptions[i].sdl_avail ) { npos += fprintf(stderr, sdl_indicator); }

        const char* text = sstOptions[i].desc;
        while ( text != nullptr && *text != '\0' ) {
            /* Advance to offset */
            while ( npos < desc_start )
                npos += fprintf(stderr, " ");

            if ( strlen(text) <= desc_width ) {
                fprintf(stderr, "%s", text);
                break;
            }
            else {
                // Need to find the last space before we need to break
                int index = desc_width - 1;
                while ( index > 0 ) {
                    if ( text[index] == ' ' ) break;
                    index--;
                }

                int nwritten = fprintf(stderr, "%.*s\n", index, text);
                text += (nwritten);
                // Skip any extra spaces
                while ( *text == ' ' )
                    text++;
                npos = 0;
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

    return false; /* Should not continue */
}

int
Config::parseCmdLine(int argc, char* argv[])
{
    ConfigHelper       helper(*this);
    static const char* sst_short_options = "hv::Vn:";
    struct option      sst_long_options[nLongOpts + 1];
    for ( size_t i = 0; i < nLongOpts; i++ ) {
        sst_long_options[i] = sstOptions[i].opt;
    }
    sst_long_options[nLongOpts] = { nullptr, 0, nullptr, 0 };

    run_name = argv[0];

    // Need to see if there are model-options specified after '--'.
    // If there is, we will deal with it later and just tell getopt
    // about everything before it.  getopt does not handle -- and
    // positional arguments in a sane way.
    int end_arg_index = 0;
    for ( int i = 0; i < argc; ++i ) {
        if ( !strcmp(argv[i], "--") ) {
            end_arg_index = i;
            break;
        }
    }
    int my_argc = end_arg_index == 0 ? argc : end_arg_index;

    bool ok           = true;
    helper.clean_exit = false;
    while ( ok ) {
        int       option_index = 0;
        const int intC         = getopt_long(my_argc, argv, sst_short_options, sst_long_options, &option_index);

        if ( intC == -1 ) /* We're done */
            break;

        const char c = static_cast<char>(intC);

        // Getopt is a bit strange it how it returns information.
        // There are three cases:

        // 1 - Unknown value:  c = '?' & option_index = 0
        // 2 - long options:  c = first letter of option & option_index = index of option in sst_long_options
        // 3 - short options: c = short option letter & option_index = 0

        // This is a dumb combination of things.  They really should
        // have return c = 0 in the long option case.  As it is,
        // there's no way to differentiate a short value from the long
        // value in index 0.  Thankfully, we have a "Section Header"
        // in index 0 so we don't have to worry about that.

        // If the character returned from getopt_long is '?', then it
        // is an unknown command
        if ( c == '?' ) {
            // Unknown option
            ok = helper.usage();
        }
        else if ( option_index != 0 ) {
            // Long options
            if ( optarg == nullptr ) {
                ok = (helper.*sstOptions[option_index].flagFunc)();
                if ( ok ) sstOptions[option_index].set_cmdline = true;
            }
            else {
                ok = (helper.*sstOptions[option_index].argFunc)(optarg);
                if ( ok ) sstOptions[option_index].set_cmdline = true;
            }
        }
        else {
            switch ( c ) {
            case 0:
                // This shouldn't happen, so we'll error if it does
                fprintf(stderr, "INTERNAL ERROR: error parsing command line options\n");
                exit(1);
                break;

            // Note, for these cases, we have to supply the
            // option_index ourselves because it is set to 0 from
            // getopt.  So, these need to be revisited if there is
            // ever a new option put above any of these options.
            case 'v':
                if ( optarg == nullptr ) { ok = helper.incrVerbose(); }
                else {
                    ok = helper.setVerbosity(optarg);
                }
                if ( ok ) sstOptions[4].set_cmdline = true;
                break;

            case 'n':
            {
                ok = helper.setNumThreads(optarg);
                if ( ok ) sstOptions[5].set_cmdline = true;
                break;
            }

            case 'V':
                ok                = helper.printVersion();
                helper.clean_exit = true;
                break;

            case 'h':
                helper.clean_exit = true;
            /* fall through */
            default:
                ok = helper.usage();
            }
        }
    }

    if ( !ok ) {
        if ( helper.clean_exit ) return 1;
        return -1;
    }

    /* Handle positional arguments */
    if ( optind < argc ) {
        // First positional argument is the sdl-file
        ok = helper.setConfigFile(argv[optind++]);

        // If there are additional position arguments, this is an
        // error
        if ( optind == (my_argc - 1) ) {
            fprintf(
                stderr, "Error: sdl-file name is the only positional argument allowed.  See help for --model-options "
                        "for more info on passing parameters to the input script.\n");
            ok = false;
        }
    }

    // Check to make sure we had an sdl-file specified
    if ( configFile_ == "NONE" ) {
        fprintf(stderr, "ERROR: no sdl-file specified\n");
        fprintf(stderr, "Usage: %s sdl-file [options]\n", run_name.c_str());
        return -1;
    }

    // Support further additional arguments specified after -- to be
    // args to the model.
    if ( end_arg_index != 0 ) {
        for ( int i = end_arg_index + 1; i < argc; ++i ) {
            helper.setModelOptions(argv[i]);
        }
    }

    if ( !ok ) return -1;

    /* Sanity check, and other duties */
    Output::setFileName(debugFile_ != "/dev/null" ? debugFile_ : "sst_output");


    // Ensure output directory ends with a directory separator
    if ( output_directory_.size() > 0 ) {
        if ( '/' != output_directory_[output_directory_.size() - 1] ) { output_directory_.append("/"); }
    }

    // Now make sure all the files we are generating go into a directory
    if ( output_config_graph_.size() > 0 && isFileNameOnly(output_config_graph_) ) {
        output_config_graph_.insert(0, output_directory_);
    }

    if ( output_dot_.size() > 0 && isFileNameOnly(output_dot_) ) { output_dot_.insert(0, output_directory_); }

    if ( output_json_.size() > 0 && isFileNameOnly(output_json_) ) { output_json_.insert(0, output_directory_); }

    if ( debugFile_.size() > 0 && isFileNameOnly(debugFile_) ) { debugFile_.insert(0, output_directory_); }

    return 0;
}


bool
Config::setOptionFromModel(const string& entryName, const string& value)
{
    ConfigHelper helper(*this);
    for ( size_t i = 0; i < nLongOpts; i++ ) {
        if ( !entryName.compare(sstOptions[i].opt.name) ) {
            if ( sstOptions[i].sdl_avail ) {
                // If this was set on the command line, skip it
                if ( sstOptions[i].set_cmdline ) return false;
                return (helper.*sstOptions[i].argFunc)(value);
            }
            else {
                fprintf(stderr, "ERROR: Option \"%s\" is not available to be set in the SDL file\n", entryName.c_str());
                exit(-1);
                return false;
            }
        }
    }
    fprintf(stderr, "ERROR: Unknown configuration entry \"%s\"\n", entryName.c_str());
    exit(-1);
    return false;
}


} // namespace SST
