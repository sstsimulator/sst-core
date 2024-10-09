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

#include "sst_config.h"

#include "sst/core/config.h"

#include "sst/core/env/envquery.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/warnmacros.h"

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace SST {

//// Helper class for setting options

class ConfigHelper
{
public:
    // Print usage
    static int printUsage(Config* cfg, const std::string& UNUSED(arg)) { return cfg->printUsage(); }

    // Print usage
    static int printHelp(Config* cfg, const std::string& arg)
    {
        if ( arg != "" ) return cfg->printExtHelp(arg);
        return cfg->printUsage();
    }

    // Prints the SST version
    static int printVersion(Config* UNUSED(cfg), const std::string& UNUSED(arg))
    {
        printf("SST-Core Version (" PACKAGE_VERSION);
        if ( strcmp(SSTCORE_GIT_HEADSHA, PACKAGE_VERSION) ) {
            printf(", git branch : " SSTCORE_GIT_BRANCH);
            printf(", SHA: " SSTCORE_GIT_HEADSHA);
        }
        printf(")\n");

        return 1; /* Should not continue, but clean exit */
    }

    // num_threads
    static int setNumThreads(Config* cfg, const std::string& arg)
    {
        try {
            unsigned long val = stoul(arg);
            cfg->num_threads_ = val;
            return 0;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --num-threads\n", arg.c_str());
            return -1;
        }
    }

    // SDL file
    static int setConfigFile(Config* cfg, const std::string& arg)
    {
        cfg->configFile_ = arg;
        return 0;
    }

    // model_options
    static int setModelOptions(Config* cfg, const std::string& arg)
    {
        if ( cfg->model_options_.empty() )
            cfg->model_options_ = arg;
        else
            cfg->model_options_ += std::string(" \"") + arg + std::string("\"");
        return 0;
    }

    // print timing
    static int setPrintTiming(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            cfg->print_timing_ = true;
            return 0;
        }
        bool success       = false;
        cfg->print_timing_ = cfg->parseBoolean(arg, success, "enable-print-timing");
        if ( success ) return 0;
        return -1;
    }

    // stop-at
    static int setStopAt(Config* cfg, const std::string& arg)
    {
        cfg->stop_at_ = arg;
        return 0;
    }


    // exit after
    static int setExitAfter(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) { return 0; }
        bool success     = false;
        cfg->exit_after_ = cfg->parseWallTimeToSeconds(arg, success, "--exit-after");
        if ( success ) return 0;
        return -1;
    }


    // partitioner
    static int setPartitioner(Config* cfg, const std::string& arg)
    {
        cfg->partitioner_ = arg;
        if ( cfg->partitioner_.find('.') == cfg->partitioner_.npos ) { cfg->partitioner_ = "sst." + cfg->partitioner_; }
        return 0;
    }

    // heart beat
    static int setHeartbeatSimPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) { return 0; }

        try {
            UnitAlgebra check(arg);
            if ( !check.hasUnits("s") && !check.hasUnits("Hz") ) {
                fprintf(
                    stderr,
                    "Error parsing option: Units passed to --heartbeat-sim-period must be time (s or Hz, SI prefix "
                    "OK). Argument = "
                    "[%s]\n",
                    arg.c_str());
                return -1;
            }
        }
        catch ( UnitAlgebra::InvalidUnitType const& ) {
            fprintf(
                stderr, "Error parsing option: Invalid units passed to --heartbeat-sim-period. Argument = [%s]\n",
                arg.c_str());
            return -1;
        }
        catch ( ... ) {
            fprintf(
                stderr,
                "Error parsing option: Argument passed to --heartbeat-sim-period cannot be parsed. Argument = [%s]\n",
                arg.c_str());
            return -1;
        }

        cfg->heartbeat_sim_period_ = arg;
        return 0;
    }

    static int setHeartbeatWallPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) { return 0; }

        bool success                = false;
        cfg->heartbeat_wall_period_ = cfg->parseWallTimeToSeconds(arg, success, "--heartbeat-wall-period");
        if ( success ) return 0;
        return -1;
    }

    // output directory
    static int setOutputDir(Config* cfg, const std::string& arg)
    {
        cfg->output_directory_ = arg;
        return 0;
    }

    // core output prefix
    static int setOutputPrefix(Config* cfg, const std::string& arg)
    {
        cfg->output_core_prefix_ = arg;
        return 0;
    }


    // Configuration output

    // output config python
    static int setWriteConfig(Config* cfg, const std::string& arg)
    {
        cfg->output_config_graph_ = arg;
        return 0;
    }

    // output config json
    static int setWriteJSON(Config* cfg, const std::string& arg)
    {
        cfg->output_json_ = arg;
        return 0;
    }

    // parallel output
#ifdef SST_CONFIG_HAVE_MPI
    static int enableParallelOutput(Config* cfg, const std::string& arg)
    {
        // If this is only a one rank job, then we can ignore
        if ( cfg->num_ranks_ == 1 ) return 0;

        bool state = true;
        // If there's an arg, we need to parse it.  Otherwise, it will
        // just get set to true.
        if ( arg != "" ) {
            bool success;
            state = cfg->parseBoolean(arg, success, "parallel-output");
            if ( !success ) return -1;
        }

        cfg->parallel_output_ = state;

        // For parallel output, we always need to output the partition
        // info as well.  Also, if it was already set to true, don't
        // overwrite even if parallel_output was set to false.
        cfg->output_partition_ = cfg->output_partition_ | cfg->parallel_output_;
        return 0;
    }
#endif


    // Graph output

    // graph output dot
    static int setWriteDot(Config* cfg, const std::string& arg)
    {
        cfg->output_dot_ = arg;
        return 0;
    }

    // dot verbosity
    static int setDotVerbosity(Config* cfg, const std::string& arg)
    {
        try {
            unsigned long val   = stoul(arg);
            cfg->dot_verbosity_ = val;
            return 0;
        }
        catch ( std::invalid_argument& e ) {
            fprintf(stderr, "Failed to parse '%s' as number for option --dot-verbosity\n", arg.c_str());
            return -1;
        }
    }

    // output partition
    static int setWritePartitionFile(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) { cfg->output_partition_ = true; }
        else {
            cfg->component_partition_file_ = arg;
        }
        return 0;
    }

    // parallel load
#ifdef SST_CONFIG_HAVE_MPI
    static int enableParallelLoadMode(Config* cfg, const std::string& arg)
    {
        // If this is only a one rank job, then we can ignore
        if ( cfg->num_ranks_ == 1 ) return 0;

        if ( arg == "" ) {
            cfg->parallel_load_ = true;
            return 0;
        }

        std::string arg_lower(arg);
        std::locale loc;
        for ( auto& ch : arg_lower )
            ch = std::tolower(ch, loc);

        if ( arg_lower == "none" ) {
            cfg->parallel_load_ = false;
            return 0;
        }
        else
            cfg->parallel_load_ = true;

        if ( arg_lower == "single" )
            cfg->parallel_load_mode_multi_ = false;
        else if ( arg_lower == "multi" )
            cfg->parallel_load_mode_multi_ = true;
        else {
            fprintf(
                stderr, "Invalid option '%s' passed to --parallel-load.  Valid options are NONE, SINGLE and MULTI.\n",
                arg.c_str());
            return -1;
        }

        return 0;
    }
#endif

    // Advanced options

    // timebase
    static int setTimebase(Config* cfg, const std::string& arg)
    {
        try {
            UnitAlgebra check(arg);
            if ( !check.hasUnits("s") && !check.hasUnits("Hz") ) {
                fprintf(
                    stderr,
                    "Error parsing option: Units passed to --timebase must be time (s or Hz). Argument = [%s]\n",
                    arg.c_str());
                return -1;
            }
        }
        catch ( UnitAlgebra::InvalidUnitType const& ) {
            fprintf(stderr, "Error parsing option: Invalid units passed to --timebase. Argument = [%s]\n", arg.c_str());
            return -1;
        }
        catch ( ... ) {
            fprintf(
                stderr, "Error parsing option: Argument passed to --timebase cannot be parsed. Argument = [%s]\n",
                arg.c_str());
            return -1;
        }
        cfg->timeBase_ = arg;
        return 0;
    }

    // timevortex
    static int setTimeVortex(Config* cfg, const std::string& arg)
    {
        cfg->timeVortex_ = arg;
        return 0;
    }

    // interthread links
    static int setInterThreadLinks(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            cfg->interthread_links_ = true;
            return 0;
        }

        bool success            = false;
        cfg->interthread_links_ = cfg->parseBoolean(arg, success, "interthread-links");
        return success ? 0 : -1;
    }

#ifdef USE_MEMPOOL
    // cache align mempool allocations
    static int setCacheAlignMempools(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            cfg->cache_align_mempools_ = true;
            return 0;
        }
        bool success               = false;
        cfg->cache_align_mempools_ = cfg->parseBoolean(arg, success, "cache-align-mempools");
        return success ? 0 : -1;
    }
#endif

    // debug file
    static int setDebugFile(Config* cfg, const std::string& arg)
    {
        cfg->debugFile_ = arg;
        return 0;
    }


    // Advanced options - profiling
    static int enableProfiling(Config* cfg, const std::string& arg)
    {
        if ( cfg->enabled_profiling_ != "" ) { cfg->enabled_profiling_ += ";"; }
        cfg->enabled_profiling_ += arg;
        return 0;
    }

    static int setProfilingOutput(Config* cfg, const std::string& arg)
    {
        cfg->profiling_output_ = arg;
        return 0;
    }

    static std::string getTimebaseExtHelp()
    {
        std::string msg = "Timebase:\n\n";
        msg.append("Time in SST core is represented by a 64-bit unsigned integer.  By default, each count of that "
                   "value represents 1ps of time.  The timebase option allows you to set that atomic core timebase to "
                   "a different value.\n ");
        msg.append("There are two things to balance when determing a timebase to use:\n\n");
        msg.append("1) The shortest time period or fastest clock frequency you want to represent:\n");
        msg.append("  It is recommended that the core timebase be set to ~1000x smaller than the shortest time period "
                   "(fastest frequency) in your simulation.  For the default 1ps timebase, clocks in the 1-10GHz range "
                   "are easily represented.  If you want to have higher frequency clocks, you can set the timebase to "
                   "a smaller value, at the cost of decreasing the amount of time you can simulate.\n\n");
        msg.append("2) How much simulated time you need to support:\n");
        msg.append("  The default timebase of 1ps will support ~215.5 days (5124 hours) of simulated time.  If you are "
                   "using SST to simulate longer term phenomena, you will need to make the core timebase longer.  A "
                   "consequence of increaing the timebase is that the minimum time period that can be represented will "
                   "increase (conversely, the maximum frequency that can be represented will increase).");
        return msg;
    }

    static std::string getProfilingExtHelp()
    {
        std::string msg = "Profiling Points [EXPERIMENTAL]:\n\n";
        msg.append(
            "NOTE: Profiling points are still in development and syntax for enabling profiling tools, as well as "
            "available profiling points is subject to change.  However, it is intended that profiling points "
            "will continue to be supported into the future.\n\n");
        msg.append("  Profiling points are points in the code where a profiling tool can be instantiated.  The "
                   "profiling tool allows you to collect various data about code segments.  There are currently three "
                   "profiling points in SST core:\n");
        msg.append("   - clock: profiles calls to user registered clock handlers\n");
        msg.append("   - event: profiles calls to user registered event handlers set on Links\n");
        msg.append("   - sync: profiles calls into the SyncManager (only valid for parallel simulations)\n");
        msg.append("\n");
        msg.append("  The format for enabling profile point is a semicolon separated list where each item specifies "
                   "details for a given profiling tool using the following format:\n");
        msg.append("   name:type(params)[point]\n");
        msg.append("     name: name of tool to be shown in output\n");
        msg.append("     type: type of profiling tool in ELI format (lib.type)\n");
        msg.append("     params: optional parameters to pass to profiling tool, format is key=value,key=value...\n");
        msg.append("     point: profiling point to load the tool into\n");
        msg.append("\n");
        msg.append("Profiling tools can all be enabled in a single instance of --enable-profiling, or you can use "
                   "multiple instances of --enable-profiling to enable more than one profiling tool.  It "
                   "is also possible to attach more than one profiling tool to a given profiling point.\n");
        msg.append("\n");
        msg.append("Examples:\n");
        msg.append(
            "  --enable-profiling=\"events:sst.profile.handler.event.time.high_resolution(level=component)[event]\"\n");
        msg.append("  --enable-profiling=\"clocks:sst.profile.handler.clock.count(level=subcomponent)[clock]\"\n");
        msg.append("  --enable-profiling=sync:sst.profile.sync.time.steady[sync]\n");
        return msg;
    }

    // Advanced options - debug

    // run mode
    static int setRunMode(Config* cfg, const std::string& arg)
    {
        if ( !arg.compare("init") )
            cfg->runMode_ = SimulationRunMode::INIT;
        else if ( !arg.compare("run") )
            cfg->runMode_ = SimulationRunMode::RUN;
        else if ( !arg.compare("both") )
            cfg->runMode_ = SimulationRunMode::BOTH;
        else {
            fprintf(stderr, "Unknown option for --run-mode: %s\n", arg.c_str());
            cfg->runMode_ = SimulationRunMode::UNKNOWN;
        }

        return cfg->runMode_ != SimulationRunMode::UNKNOWN ? 0 : -1;
    }

    static int setInteractiveConsole(Config* cfg, const std::string& arg)
    {
        cfg->interactive_console_ = arg;
        return 0;
    }

    static int setInteractiveStartTime(Config* cfg, const std::string& arg)
    {
        if ( arg == "" )
            cfg->interactive_start_time_ = "0";
        else
            cfg->interactive_start_time_ = arg;
        return 0;
    }

    // dump undeleted events
#ifdef USE_MEMPOOL
    static int setWriteUndeleted(Config* cfg, const std::string& arg)
    {
        cfg->event_dump_file_ = arg;
        return 0;
    }
#endif

    // rank sequentional startup
    static int forceRankSeqStartup(Config* cfg, const std::string& UNUSED(arg))
    {
        cfg->rank_seq_startup_ = true;
        return 0;
    }

    // Advanced options - checkpointing

    // Set frequency of checkpoint generation
    static int setCheckpointWallPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) { return 0; }
        bool success                 = false;
        cfg->checkpoint_wall_period_ = cfg->parseWallTimeToSeconds(arg, success, "--checkpoint-wall-period");
        if ( success ) return 0;
        return -1;
    }

    static int setCheckpointSimPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) { return 0; }

        try {
            UnitAlgebra check(arg);
            if ( check.hasUnits("s") || check.hasUnits("Hz") ) {
                cfg->checkpoint_sim_period_ = arg;
                return 0;
            }
            else {
                fprintf(
                    stderr,
                    "Error parsing option: --checkpoint-sim-period requires time units (s or Hz, SI prefix OK). "
                    "Argument = [%s]\n",
                    arg.c_str());
                return -1;
            }
        }
        catch ( UnitAlgebra::InvalidUnitType& e ) {
            fprintf(
                stderr,
                "Error parsing option: Argument passed to --checkpoint-sim-period has invalid units. Units must be "
                "time (s "
                "or Hz, SI prefix OK). Argument = [%s]\n",
                arg.c_str());
            return -1;
        }
        catch ( ... ) { /* Fall through */
        }

        fprintf(
            stderr,
            "Error parsing option: Argument passed to --checkpoint-sim-period could not be parsed. Argument = [%s]\n",
            arg.c_str());
        return -1;
    }

    // Set whether to load from checkpoint
    static int setLoadFromCheckpoint(Config* cfg, const std::string& UNUSED(arg))
    {
        cfg->load_from_checkpoint_ = true;
        return 0;
    }

    // Set the prefix for checkpoint files
    static int setCheckpointPrefix(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            fprintf(stderr, "Error, checkpoint-prefix must not be an empty string\n");
            return -1;
        }
        cfg->checkpoint_prefix_ = arg;
        return 0;
    }

    static std::string getCheckpointPrefixExtHelp()
    {
        std::string msg = "Checkpointing:\n\n";
        msg.append("The checkpoint prefix is used in the naming of the directories and files created by the "
                   "checkpoint engine.  If no checkpoint prefix is set, sst will simply use \"checkpoint\"."
                   "In the following explanation, <prefix> will be used to represent the "
                   "prefix set with the --checkpoint-prefix option.  On sst start, the checkpoint engine will "
                   "create a directory with the name <prefix> to hold all the checkpoint files.  If <prefix> "
                   "already exists, the it will append _N, where N starts at 1 and increases by one until a "
                   "directory name that doesn't already exist is reached (i.e. <prefix>_1, <prefix>_2, etc.).\n");

        msg.append("\nWithin the checkpoint directory, each checkpoint will create its own subdirectory with "
                   "the form <prefix>_<checkpoint_id>_<simulated_time>, where checkpoint_id starts at 0 and "
                   "increments by one for each checkpoint.  Within this directory, there are three types of "
                   "files:\n\n");

        msg.append("Registry file: The file containes a list of some "
                   "of the global parameters from the sst run, followed by a list of all other files that "
                   "are a part of the checkpoint. The two files, described below, are the globals file and "
                   "the serialized data from each of the threads in the simulation.  After each of the serialized "
                   "data files, each Component that was in that partition is listed, along with its offset to the "
                   "location in the file for the Components serialized data. this file is named the same as the "
                   "directory with a .sstcpt extension:\n"
                   "    <prefix>_<checkpoint_id>_<simulated_time>.sstcpt.\n\n");

        msg.append("Globals file: This contains the serialized binary data needed at sst startup time that is "
                   "needed by all partitions. This file is named:\n"
                   "    <prefix>_<checkpoint_id>_<simulated time>_globals.bin\n\n");

        msg.append("Serialized data files: these are the files that hold all of the data for each thread of "
                   "execution in the original run.  The files are named by rank:\n"
                   "    <prefix>_<checkpoint_id>_<simulated_time>_<rank>_<thread>.bin\n\n");

        msg.append("A sample directory structure using a checkpoint prefix of \"checkpoint\" using two ranks "
                   "with one thread each would look something like:\n\n"
                   "current working directory\n"
                   "|--checkpoint\n"
                   "   |--checkpoint_0_1000\n"
                   "      |--checkpoint_0_1000.sstcpt\n"
                   "      |--checkpoint_0_1000_globals.bin\n"
                   "      |--checkpoint_0_1000_0_0.bin\n"
                   "      |--checkpoint_0_1000_1_0.bin\n"
                   "   |--checkpoint_1_2000\n"
                   "      |--checkpoint_1_2000.sstcpt\n"
                   "      |--checkpoint_1_2000_globals.bin\n"
                   "      |--checkpoint_1_2000_0_0.bin\n"
                   "      |--checkpoint_1_2000_1_0.bin\n\n");

        msg.append("When restarting from a checkpoint, the registry file (*.sstcpt) should be specified as the "
                   "input file.\n");

        return msg;
    }

    // Advanced options - environment

    // Disable signal handlers
    static int disableSigHandlers(Config* cfg, const std::string& UNUSED(arg))
    {
        cfg->enable_sig_handling_ = false;
        return 0;
    }

    // Set SIGUSR1 handler
    static int setSigUsr1(Config* cfg, const std::string& arg)
    {
        cfg->sigusr1_ = arg;
        return 0;
    }

    // Set SIGUSR2 handler
    static int setSigUsr2(Config* cfg, const std::string& arg)
    {
        cfg->sigusr2_ = arg;
        return 0;
    }

    // Set SIGALRM handler(s)
    static int setSigAlrm(Config* cfg, const std::string& arg)
    {
        if ( cfg->sigalrm_ != "" ) { cfg->sigalrm_ += ";"; }
        cfg->sigalrm_ += arg;
        return 0;
    }

    // Extended help for SIGALRM
    static std::string getSignalExtHelp()
    {
        std::string msg = "RealTime Actions [EXPERIMENTAL]:\n\n";
        msg.append("  RealTimeActions are actions that execute in response to system signals SIGUSR1, SIGUSR2, and/or "
                   "SIGALRM. "
                   "The following actions are available from SST core or custom actions may also be defined.\n"
                   "   - sst.rt.exit.clean: Exits SST normally.\n"
                   "   - sst.rt.exit.emergency: Exists SST in an emergency state. Triggered on SIGINT and SIGTERM.\n"
                   "   - sst.rt.status.core: Reports brief state of SST core.\n"
                   "   - sst.rt.status.all: Reports state of SST core and every simulated component.\n"
                   "   - sst.rt.checkpoint: Creates a checkpoint.\n"
                   "   - sst.rt.heartbeat: Reports state of SST core and some profiling state (e.g., memory usage).\n");
        msg.append(
            "  An action can be attached to SIGUSR1 using '--sigusr1=<handler>' and SIGUSR2 using '--sigusr2=<handler>'"
            " If not specified SST uses the defaults: --sigusr1=sst.rt.status.core and --sigusr2=sst.rt.status.all.\n");
        msg.append("  Actions can be bound to SIGALRM by specifying '--sigalrm=ACTION(interval=TIME)' where ACTION is "
                   "the action and TIME is a wall-clock time in the format HH:MM:SS, MM:SS, SS, Hh, Mm, or Ss. Capital "
                   "letters represent numerics and lower case are units and required for those formats. Multiple "
                   "actions can be separated by semicolons or multiple instances of --sigalrm can be used.\n");
        msg.append("  Examples:\n");
        msg.append("    --sigusr1=sst.rt.checkpoint\n");
        msg.append("    --sigusr2=sst.rt.heartbeat\n");
        msg.append("    --sigalrm=\"sst.rt.checkpoint(interval=2h);sst.rt.heartbeat(interval=30m)\"\n");
        return msg;
    }
};


//// Config Class functions

void
Config::print()
{
    std::cout << "verbose = " << verbose_ << std::endl;
    std::cout << "num_threads = " << num_threads_ << std::endl;
    std::cout << "num_ranks = " << num_ranks_ << std::endl;
    std::cout << "configFile = " << configFile_ << std::endl;
    std::cout << "model_options = " << model_options_ << std::endl;
    std::cout << "print_timing = " << print_timing_ << std::endl;
    std::cout << "stop_at = " << stop_at_ << std::endl;
    std::cout << "exit_after = " << exit_after_ << std::endl;
    std::cout << "partitioner = " << partitioner_ << std::endl;
    std::cout << "heartbeat_wall_period = " << heartbeat_wall_period_ << std::endl;
    std::cout << "heartbeat_sim_period = " << heartbeat_sim_period_ << std::endl;
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
    std::cout << "load_checkpoint = " << load_from_checkpoint_ << std::endl;
    std::cout << "checkpoint_wall_period = " << checkpoint_wall_period_ << std::endl;
    std::cout << "checkpoint_sim_period = " << checkpoint_sim_period_ << std::endl;
    std::cout << "checkpoint_prefix = " << checkpoint_prefix_ << std::endl;
    std::cout << "timeVortex = " << timeVortex_ << std::endl;
    std::cout << "interthread_links = " << interthread_links_ << std::endl;
#ifdef USE_MEMPOOL
    std::cout << "cache_align_mempools = " << cache_align_mempools_ << std::endl;
#endif
    std::cout << "debugFile = " << debugFile_ << std::endl;
    std::cout << "libpath = " << libpath_ << std::endl;
    std::cout << "addLlibPath = " << addlibpath_ << std::endl;
    std::cout << "enabled_profiling = " << enabled_profiling_ << std::endl;
    std::cout << "profiling_output = " << profiling_output_ << std::endl;

    switch ( runMode_ ) {
    case SimulationRunMode::INIT:
        std::cout << "runMode = INIT" << std::endl;
        break;
    case SimulationRunMode::RUN:
        std::cout << "runMode = RUN" << std::endl;
        break;
    case SimulationRunMode::BOTH:
        std::cout << "runMode = BOTH" << std::endl;
        break;
    default:
        std::cout << "runMode = UNKNOWN" << std::endl;
        break;
    }

    std::cout << "interactive_console = " << interactive_console_ << std::endl;
    std::cout << "interactive_start_time = " << interactive_start_time_ << std::endl;

#ifdef USE_MEMPOOL
    std::cout << "event_dump_file = " << event_dump_file_ << std::endl;
#endif
    std::cout << "rank_seq_startup_ " << rank_seq_startup_ << std::endl;
    std::cout << "print_env" << print_env_ << std::endl;
    std::cout << "enable_sig_handling = " << enable_sig_handling_ << std::endl;
    std::cout << "sigusr1 = " << sigusr1_ << std::endl;
    std::cout << "sigusr2 = " << sigusr2_ << std::endl;
    std::cout << "sigalrm = " << sigalrm_ << std::endl;
    std::cout << "no_env_config = " << no_env_config_ << std::endl;
}

static std::vector<AnnotationInfo> annotations = {
    { 'S',
      "Options annotated with 'S' can be set in the SDL file (input configuration file)\n  - Note: Options set on the "
      "command line take precedence over options set in the SDL file\n" }
};

Config::Config(uint32_t num_ranks, bool first_rank) : ConfigShared(!first_rank, annotations)
{
    // Basic Options
    first_rank_ = first_rank;

    num_ranks_             = num_ranks;
    num_threads_           = 1;
    configFile_            = "NONE";
    model_options_         = "";
    print_timing_          = false;
    stop_at_               = "0 ns";
    exit_after_            = 0;
    partitioner_           = "sst.linear";
    heartbeat_sim_period_  = "";
    heartbeat_wall_period_ = 0;

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
    debugFile_ = "/dev/null";

    // Advance Options - Profiling
    enabled_profiling_ = "";
    profiling_output_  = "stdout";

    // Advanced Options - Debug
    runMode_                = SimulationRunMode::BOTH;
    interactive_console_    = "";
    interactive_start_time_ = "";
#ifdef USE_MEMPOOL
    event_dump_file_ = "";
#endif
    rank_seq_startup_ = false;

    // Advanced Options - Checkpointing
    checkpoint_wall_period_ = 0;
    checkpoint_sim_period_  = "";
    load_from_checkpoint_   = false;
    checkpoint_prefix_      = "checkpoint";

    // Advanced Options - environment
    enable_sig_handling_ = true;
    sigusr1_             = "sst.rt.status.core";
    sigusr2_             = "sst.rt.status.all";
    sigalrm_             = "";

    insertOptions();
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

void
Config::insertOptions()
{
    using namespace std::placeholders;
    /* Informational options */
    DEF_SECTION_HEADING("Informational Options");
    DEF_FLAG("usage", 'h', "Print usage information.", std::bind(&ConfigHelper::printUsage, this, _1));
    DEF_ARG(
        "help", 0, "option", "Print extended help information for requested option.",
        std::bind(&ConfigHelper::printHelp, this, _1), false);
    DEF_FLAG("version", 'V', "Print SST Release Version", std::bind(&ConfigHelper::printVersion, this, _1));

    /* Basic Options */
    DEF_SECTION_HEADING("Basic Options (most commonly used)");
    addVerboseOptions(true);
    DEF_ARG(
        "num-threads", 'n', "NUM", "Number of parallel threads to use per rank",
        std::bind(&ConfigHelper::setNumThreads, this, _1), true);
    DEF_ARG(
        "sdl-file", 0, "FILE",
        "Specify SST Configuration file.  Note: this is most often done by just specifying the file without an option.",
        std::bind(&ConfigHelper::setConfigFile, this, _1), false);
    DEF_ARG(
        "model-options", 0, "STR",
        "Provide options to the python configuration script.  Additionally, any arguments provided after a final '-- ' "
        "will be appended to the model options (or used as the model options if --model-options was not specified).",
        std::bind(&ConfigHelper::setModelOptions, this, _1), false);
    DEF_FLAG_OPTVAL(
        "print-timing-info", 0, "Print SST timing information", std::bind(&ConfigHelper::setPrintTiming, this, _1),
        true);
    DEF_ARG(
        "stop-at", 0, "TIME", "Set time at which simulation will end execution",
        std::bind(&ConfigHelper::setStopAt, this, _1), true);
    DEF_ARG(
        "exit-after", 0, "TIME",
        "Set the maximum wall time after which simulation will end execution.  Time is specified in hours, minutes and "
        "seconds, with the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the "
        "appropriate numbers for that value, lower case letters represent the units and are required for those "
        "formats).",
        std::bind(&ConfigHelper::setExitAfter, this, _1), true);
    DEF_ARG(
        "partitioner", 0, "PARTITIONER", "Select the partitioner to be used. <lib.partitionerName>",
        std::bind(&ConfigHelper::setPartitioner, this, _1), true);
    DEF_ARG(
        "heartbeat-period", 0, "PERIOD",
        "Set time for heartbeats to be published (these are approximate timings measured in simulation time, published "
        "by the core, to update on progress)",
        std::bind(&ConfigHelper::setHeartbeatSimPeriod, this, _1), true);
    DEF_ARG(
        "heartbeat-wall-period", 0, "PERIOD",
        "Set approximate frequency for heartbeats (SST-Core progress updates) to be published in terms of wall (real) "
        "time. PERIOD can be specified in hours, minutes, and seconds with "
        "the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the appropriate numbers for "
        "that value, lower case letters represent the units and are required for those formats.).",
        std::bind(&ConfigHelper::setHeartbeatWallPeriod, this, _1), true);
    DEF_ARG(
        "heartbeat-sim-period", 0, "PERIOD",
        "Set approximate frequency for heartbeats (SST-Core progress updates) to be published in terms of simulated "
        "time. PERIOD must include time units (s or Hz) and SI prefixes are accepted.",
        std::bind(&ConfigHelper::setHeartbeatSimPeriod, this, _1), true);
    DEF_ARG(
        "output-directory", 0, "DIR", "Directory into which all SST output files should reside",
        std::bind(&ConfigHelper::setOutputDir, this, _1), true);
    DEF_ARG(
        "output-prefix-core", 0, "STR", "set the SST::Output prefix for the core",
        std::bind(&ConfigHelper::setOutputPrefix, this, _1), true);

    /* Configuration Output */
    DEF_SECTION_HEADING(
        "Configuration Output Options (generates a file that can be used as input for reproducing a run)");
    DEF_ARG(
        "output-config", 0, "FILE", "File to write SST configuration (in Python format)",
        std::bind(&ConfigHelper::setWriteConfig, this, _1), true);
    DEF_ARG(
        "output-json", 0, "FILE", "File to write SST configuration graph (in JSON format)",
        std::bind(&ConfigHelper::setWriteJSON, this, _1), true);
#ifdef SST_CONFIG_HAVE_MPI
    DEF_FLAG_OPTVAL(
        "parallel-output", 0,
        "Enable parallel output of configuration information.  This option is ignored for single rank jobs.  Must also "
        "specify an output type (--output-config and/or --output-json).  Note: this will also cause partition info to "
        "be output if set to true.",
        std::bind(&ConfigHelper::enableParallelOutput, this, _1), true);
#endif

    /* Configuration Output */
    DEF_SECTION_HEADING("Graph Output Options (for outputting graph information for visualization or inspection)");
    DEF_ARG(
        "output-dot", 0, "FILE", "File to write SST configuration graph (in GraphViz format)",
        std::bind(&ConfigHelper::setWriteDot, this, _1), true);
    DEF_ARG(
        "dot-verbosity", 0, "INT", "Amount of detail to include in the dot graph output",
        std::bind(&ConfigHelper::setDotVerbosity, this, _1), true);
    DEF_ARG_OPTVAL(
        "output-partition", 0, "FILE",
        "File to write SST component partitioning information.  When used without an argument and in conjuction with "
        "--output-json or --output-config options, will cause paritition information to be added to graph output.",
        std::bind(&ConfigHelper::setWritePartitionFile, this, _1), true);

    /* Advanced Features */
    DEF_SECTION_HEADING("Advanced Options");
    DEF_ARG_EH(
        "timebase", 0, "TIMEBASE", "Set the base time step of the simulation (default: 1ps)",
        std::bind(&ConfigHelper::setTimebase, this, _1), std::bind(&ConfigHelper::getTimebaseExtHelp), true);
#ifdef SST_CONFIG_HAVE_MPI
    DEF_ARG_OPTVAL(
        "parallel-load", 0, "MODE",
        "Enable parallel loading of configuration. This option is ignored for single rank jobs.  Optional mode "
        "parameters are NONE, SINGLE and MULTI (default).  If NONE is specified, parallel-load is turned off. If "
        "SINGLE is specified, the same file will be passed to all MPI ranks.  If MULTI is specified, each MPI rank is "
        "required to have it's own file to load. Note, not all input "
        "formats support both types of file loading.",
        std::bind(&ConfigHelper::enableParallelLoadMode, this, _1), false);
#endif
    DEF_ARG(
        "timeVortex", 0, "MODULE", "Select TimeVortex implementation <lib.timevortex>",
        std::bind(&ConfigHelper::setTimeVortex, this, _1), true);
    DEF_FLAG_OPTVAL(
        "interthread-links", 0, "[EXPERIMENTAL] Set whether or not interthread links should be used",
        std::bind(&ConfigHelper::setInterThreadLinks, this, _1), true);
#ifdef USE_MEMPOOL
    DEF_FLAG_OPTVAL(
        "cache-align-mempools", 0, "[EXPERIMENTAL] Set whether mempool allocations are cache aligned",
        std::bind(&ConfigHelper::setCacheAlignMempools, this, _1), true);
#endif
    DEF_ARG(
        "debug-file", 0, "FILE", "File where debug output will go", std::bind(&ConfigHelper::setDebugFile, this, _1),
        true);
    addLibraryPathOptions();

    /* Advanced Features - Profiling */
    DEF_SECTION_HEADING("Advanced Options - Profiling (EXPERIMENTAL)");
    DEF_ARG_EH(
        "enable-profiling", 0, "POINTS",
        "Enables default profiling for the specified points.  Argument is a semicolon separated list specifying the "
        "points to enable.",
        std::bind(&ConfigHelper::enableProfiling, this, _1), std::bind(&ConfigHelper::getProfilingExtHelp), true);
    DEF_ARG(
        "profiling-output", 0, "FILE", "Set output location for profiling data [stdout (default) or a filename]",
        std::bind(&ConfigHelper::setProfilingOutput, this, _1), true);

    /* Advanced Features - Debug */
    DEF_SECTION_HEADING("Advanced Options - Debug");
    DEF_ARG(
        "run-mode", 0, "MODE", "Set run mode [ init | run | both (default)]",
        std::bind(&ConfigHelper::setRunMode, this, _1), true);
    DEF_ARG(
        "interactive-console", 0, "ACTION",
        "[EXPERIMENTAL] Set console to use for interactive mode. NOTE: This currently only works for serial jobs and "
        "this option will be ignored for parallel runs.",
        std::bind(&ConfigHelper::setInteractiveConsole, this, _1), true);
    DEF_ARG_OPTVAL(
        "interactive-start", 0, "TIME",
        "[EXPERIMENTAL] Drop into interactive mode at specified simulated time.  If no time is specified, or the time "
        "is 0, then it will "
        "drop into interactive mode before any events are processed in the main run loop. This option is ignored if no "
        "interactive console was set. NOTE: This currently only works for serial jobs and this option will be ignored "
        "for parallel runs.",
        std::bind(&ConfigHelper::setInteractiveStartTime, this, _1), true);
#ifdef USE_MEMPOOL
    DEF_ARG(
        "output-undeleted-events", 0, "FILE",
        "file to write information about all undeleted events at the end of simulation (STDOUT and STDERR can be used "
        "to output to console)",
        std::bind(&ConfigHelper::setWriteUndeleted, this, _1), true);
#endif
    DEF_FLAG(
        "force-rank-seq-startup", 0,
        "Force startup phases of simulation to execute one rank at a time for debug purposes",
        std::bind(&ConfigHelper::forceRankSeqStartup, this, _1));

    /* Advanced Features - Environment Setup/Reporting */
    DEF_SECTION_HEADING("Advanced Options - Environment Setup/Reporting");
    addEnvironmentOptions();
    DEF_FLAG(
        "disable-signal-handlers", 0, "Disable signal handlers",
        std::bind(&ConfigHelper::disableSigHandlers, this, _1));
    DEF_ARG_EH(
        "sigusr1", 0, "MODULE", "Select handler for SIGUSR1 signal. See extended help for detail.",
        std::bind(&ConfigHelper::setSigUsr1, this, _1), std::bind(&ConfigHelper::getSignalExtHelp), true);
    DEF_ARG_EH(
        "sigusr2", 0, "MODULE", "Select handler for SIGUSR2 signal. See extended help for detail.",
        std::bind(&ConfigHelper::setSigUsr2, this, _1), std::bind(&ConfigHelper::getSignalExtHelp), true);
    DEF_ARG_EH(
        "sigalrm", 0, "MODULE",
        "Select handler for SIGALRM signals.  Argument is a semicolon separated list specifying the "
        "handlers to register along with a time interval for each. See extended help for detail.",
        std::bind(&ConfigHelper::setSigAlrm, this, _1), std::bind(&ConfigHelper::getSignalExtHelp), true);

    /* Advanced Features - Checkpoint */
    DEF_SECTION_HEADING("Advanced Options - Checkpointing (EXPERIMENTAL)");
    DEF_ARG(
        "checkpoint-wall-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of wall (real) time. PERIOD can be "
        "specified in hours, minutes, and seconds with "
        "the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the appropriate numbers for "
        "that value, lower case letters represent the units and are required for those formats.).",
        std::bind(&ConfigHelper::setCheckpointWallPeriod, this, _1), true);
    DEF_ARG(
        "checkpoint-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of simulated time. PERIOD must include "
        "time units (s or Hz) and SI prefixes are accepted. This flag will eventually be removed in favor of "
        "--checkpoint-sim-period",
        std::bind(&ConfigHelper::setCheckpointSimPeriod, this, _1), true);
    DEF_ARG(
        "checkpoint-sim-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of simulated time. PERIOD must include "
        "time units (s or Hz) and SI prefixes are accepted.",
        std::bind(&ConfigHelper::setCheckpointSimPeriod, this, _1), true);
    DEF_FLAG(
        "load-checkpoint", 0,
        "Load checkpoint and continue simulation. Specified SDL file will be used as the checkpoint file.",
        std::bind(&ConfigHelper::setLoadFromCheckpoint, this, _1), false);
    DEF_ARG_EH(
        "checkpoint-prefix", 0, "PREFIX",
        "Set prefix for checkpoint filenames. The checkpoint prefix defaults to checkpoint if this option is not set "
        "and checkpointing is enabled.",
        std::bind(&ConfigHelper::setCheckpointPrefix, this, _1), std::bind(&ConfigHelper::getCheckpointPrefixExtHelp),
        true);

    enableDashDashSupport(std::bind(&ConfigHelper::setModelOptions, this, _1));
    addPositionalCallback(std::bind(&Config::positionalCallback, this, _1, _2));
};

std::string
Config::getUsagePrelude()
{
    std::string prelude = "Usage: sst [options] config-file\n";
    prelude.append("  Arguments to options contained in [] are optional\n");
    prelude.append("  Notes on flag options (options that take an optional BOOL value):\n");
    prelude.append("   - BOOL values can be expressed as true/false, yes/no, on/off or 1/0\n");
    prelude.append("   - Program default for flags is false\n");
    prelude.append("   - BOOL values default to true if flag is specified but no value is supplied\n");
    return prelude;
}

int
Config::positionalCallback(int num, const std::string& arg)
{
    if ( num == 0 ) {
        // First positional argument is the sdl-file
        configFile_ = arg;
    }
    else {
        // If there are additional position arguments, this is an
        // error
        fprintf(
            stderr, "Error: sdl-file name is the only positional argument allowed.  See help for --model-options "
                    "for more info on passing parameters to the input script.\n");
        return -1;
    }
    return 0;
}

int
Config::checkArgsAfterParsing()
{
    // Check to make sure we had an sdl-file specified
    if ( configFile_ == "NONE" ) {
        fprintf(stderr, "ERROR: no sdl-file specified\n");
        fprintf(stderr, "Usage: %s sdl-file [options]\n", run_name.c_str());
        return -1;
    }

    /* Sanity check, and other duties */

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
    // Check to make sure option is settable in the SDL file
    if ( getAnnotation(entryName, 'S') ) { return setOptionExternal(entryName, value); }
    fprintf(stderr, "ERROR: Option \"%s\" is not available to be set in the SDL file\n", entryName.c_str());
    exit(-1);
    return false;
}

bool
Config::canInitiateCheckpoint()
{
    if ( checkpoint_wall_period_ != 0 ) return true;
    if ( checkpoint_sim_period_ != "" ) return true;
    return false;
}

} // namespace SST
