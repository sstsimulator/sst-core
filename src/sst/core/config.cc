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

    // timing json
    static int setTimingJSON(Config* cfg, const std::string& arg)
    {
        // Sanity check to make sure filename string is not omitted
        if ( arg[0] == '-' ) {
            fprintf(stderr, "Error parsing file for --timing-info-json. Argument = [%s]\n", arg.c_str());
            return -1;
        }
        cfg->timing_json_ = arg;
        return 0;
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
        if ( arg == "" ) {
            return 0;
        }
        bool success     = false;
        cfg->exit_after_ = cfg->parseWallTimeToSeconds(arg, success, "--exit-after");
        if ( success ) return 0;
        return -1;
    }


    // partitioner
    static int setPartitioner(Config* cfg, const std::string& arg)
    {
        cfg->partitioner_ = arg;
        if ( cfg->partitioner_.find('.') == cfg->partitioner_.npos ) {
            cfg->partitioner_ = "sst." + cfg->partitioner_;
        }
        return 0;
    }

    // heart beat
    static int setHeartbeatSimPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            return 0;
        }

        try {
            UnitAlgebra check(arg);
            if ( !check.hasUnits("s") && !check.hasUnits("Hz") ) {
                fprintf(stderr,
                    "Error parsing option: Units passed to --heartbeat-sim-period must be time (s or Hz, SI prefix "
                    "OK). Argument = "
                    "[%s]\n",
                    arg.c_str());
                return -1;
            }
        }
        catch ( UnitAlgebra::InvalidUnitType const& ) {
            fprintf(stderr, "Error parsing option: Invalid units passed to --heartbeat-sim-period. Argument = [%s]\n",
                arg.c_str());
            return -1;
        }
        catch ( ... ) {
            fprintf(stderr,
                "Error parsing option: Argument passed to --heartbeat-sim-period cannot be parsed. Argument = [%s]\n",
                arg.c_str());
            return -1;
        }

        cfg->heartbeat_sim_period_ = arg;
        return 0;
    }

    static int setHeartbeatWallPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            return 0;
        }

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
        if ( arg == "" ) {
            cfg->output_partition_ = true;
        }
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
            fprintf(stderr,
                "Invalid option '%s' passed to --parallel-load.  Valid options are NONE, SINGLE and MULTI.\n",
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
                fprintf(stderr,
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
            fprintf(stderr, "Error parsing option: Argument passed to --timebase cannot be parsed. Argument = [%s]\n",
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

#if PY_MINOR_VERSION >= 9
    // enable Python coverage
    static int enablePythonCoverage(Config* cfg, const std::string& UNUSED(arg))
    {
        cfg->enable_python_coverage_ = true;
        return 0;
    }
#endif

    // Advanced options - profiling
    static int enableProfiling(Config* cfg, const std::string& arg)
    {
        if ( cfg->enabled_profiling_ != "" ) {
            cfg->enabled_profiling_ += ";";
        }
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

#if PY_MINOR_VERSION >= 9
    static std::string getPythonCoverageExtHelp()
    {
        std::string msg = "Python Coverage (EXPERIMENTAL):\n\n";

        msg.append("NOTE: This feature is considered experimental until we can complete further testing.\n\n");

        msg.append("If you are using python configuration (model definition) files as part of a larger project and are "
                   "interested in measuring code coverage of a test/example/application suite, you can instruct sst to "
                   "enable the python coverage module when launching python configuration files as part of a "
                   "simulation invocation.  To do so, you need three things:\n\n");

        msg.append("\t1.\t\vInstall python’s coverage module (via an OS package or pip) on your system "
                   "<https://pypi.org/project/coverage/>\n");

        msg.append("\t2.\t\vEnsure that the \"coverage\" command is in your path and that you can invoke the python "
                   "that SST uses and import the coverage module without error.\n");

        msg.append(
            "\t3.\t\vSet the environment variable SST_CONFIG_PYTHON_COVERAGE to a value of 1, yes, on, true or t; or "
            "invoke coverage on the command line by using the command line option --enable-python-coverage.\n\n");

        msg.append("Then invoke SST as normal using the python model configuration file for which you want to measure "
                   "coverage.\n");

        return msg;
    }
#endif

    static std::string getProfilingExtHelp()
    {
        std::string msg = "Profiling Points [API Not Yet Final]:\n\n";
        msg.append(
            "NOTE: Profiling points are still in development and syntax for enabling profiling tools is subject to "
            "change. Additional profiling points may also be added in the future.\n\n");
        msg.append("Profiling points are points in the code where a profiling tool can be instantiated.  The "
                   "profiling tool allows you to collect various data about code segments.  There are currently three "
                   "profiling points in SST core:\n\n");
        msg.append("\tclock: \t\vprofiles calls to user registered clock handlers\n");
        msg.append("\tevent: \t\vprofiles calls to user registered event handlers set on Links\n");
        msg.append("\tsync:  \t\vprofiles calls into the SyncManager (only valid for parallel simulations)\n");
        msg.append("\n");
        msg.append("The format for enabling profile point is a semicolon separated list where each item specifies "
                   "details for a given profiling tool using the following format:\n\n");
        msg.append("\tname:type(params)[point], where\n");
        msg.append("\t\tname   \t= \vname of tool to be shown in output\n");
        msg.append("\t\ttype   \t= \vtype of profiling tool in ELI format (lib.type)\n");
        msg.append(
            "\t\tparams \t= \voptional parameters to pass to profiling tool, format is key=value,key=value...\n");
        msg.append("\t\tpoint  \t= \vprofiling point to load the tool into\n");
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
        if ( arg == "" ) {
            return 0;
        }
        bool success                 = false;
        cfg->checkpoint_wall_period_ = cfg->parseWallTimeToSeconds(arg, success, "--checkpoint-wall-period");
        if ( success ) return 0;
        return -1;
    }

    static int setCheckpointSimPeriod(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            return 0;
        }

        try {
            UnitAlgebra check(arg);
            if ( check.hasUnits("s") || check.hasUnits("Hz") ) {
                cfg->checkpoint_sim_period_ = arg;
                return 0;
            }
            else {
                fprintf(stderr,
                    "Error parsing option: --checkpoint-sim-period requires time units (s or Hz, SI prefix OK). "
                    "Argument = [%s]\n",
                    arg.c_str());
                return -1;
            }
        }
        catch ( UnitAlgebra::InvalidUnitType& e ) {
            fprintf(stderr,
                "Error parsing option: Argument passed to --checkpoint-sim-period has invalid units. Units must be "
                "time (s "
                "or Hz, SI prefix OK). Argument = [%s]\n",
                arg.c_str());
            return -1;
        }
        catch ( ... ) { /* Fall through */
        }

        fprintf(stderr,
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

    // Set the prefix for checkpoint files
    static int setCheckpointNameFormat(Config* cfg, const std::string& arg)
    {
        if ( arg == "" ) {
            fprintf(stderr, "Error, checkpoint-name-format must not be an empty string\n");
            return -1;
        }
        cfg->checkpoint_name_format_ = arg;

        // Check the format string to make sure there are no more than
        // 1 slash (/) and that it only uses valid % formats
        size_t count = 0;
        size_t index = arg.find('/');
        while ( index != arg.npos ) {
            count++;
            index = arg.find('/', index + 1);
        }

        if ( count > 1 ) {
            fprintf(stderr,
                "Error parsing option: Argument passed to --checkpoint-name-format cannot "
                "have more than one directory separator (/). Argument = [%s]\n",
                arg.c_str());
            return -1;
        }

        bool valid_format  = true;
        bool percent_found = false;
        for ( auto& x : arg ) {
            if ( percent_found ) {
                if ( x != 'p' && x != 'n' && x != 't' ) {
                    // Bad format type
                    valid_format = false;
                    break;
                }
                percent_found = false;
            }
            else if ( x == '%' ) {
                percent_found = true;
            }
        }
        // If percent_found is still true, then we ended with %, which
        // is also an error
        if ( !valid_format || percent_found ) {
            fprintf(stderr,
                "Error parsing option: Argument passed to --checkpoint-name-format uses "
                "invalid control sequence. Argument = [%s]\n",
                arg.c_str());
            return -1;
        }

        return 0;
    }

    static std::string getCheckpointPrefixExtHelp()
    {
        std::string msg = "Checkpointing:\n\n";
        msg.append("The checkpoint prefix is used in the naming of the directories and files created by the "
                   "checkpoint engine.  If no checkpoint prefix is set, sst will simply use \"checkpoint\". "
                   "In the following explanation, <prefix> will be used to represent the "
                   "prefix set with the --checkpoint-prefix option.  On sst start, the checkpoint engine will "
                   "create a directory with the name <prefix> to hold all the checkpoint files.  If <prefix> "
                   "already exists, then it will append _N, where N starts at 1 and increases by one until a "
                   "directory name that doesn't already exist is reached (i.e. <prefix>_1, <prefix>_2, etc.).\n");

        msg.append("\nWithin the checkpoint directory, each checkpoint will create its own subdirectory with "
                   "a user specified name format (see extended help for --checkpoint-name-format). By default, "
                   "this is of the form <prefix>_<checkpoint_id>_<simulated_time>, where checkpoint_id starts "
                   "at 1 and increments by one for each checkpoint.  Within this directory, there are three "
                   "types of files:\n\n");

        msg.append("Registry file: The file contains a list of some "
                   "of the global parameters from the sst run, followed by a list of all other files that "
                   "are a part of the checkpoint. The two files, described below, are the globals file and "
                   "the serialized data from each of the threads in the simulation.  After each of the serialized "
                   "data files, each Component that was in that partition is listed, along with its offset to the "
                   "location in the file for the Components serialized data. This file is named using a user "
                   "specified name format (this defaults to be the same as the directory) with a .sstcpt extension:\n"
                   "    <prefix>_<checkpoint_id>_<simulated_time>.sstcpt.\n\n");

        msg.append("Globals file: This contains the serialized binary data needed at sst startup time that is "
                   "needed by all partitions. This file is named:\n"
                   "    <prefix>_<checkpoint_id>_<simulated time>_globals.bin\n\n");

        msg.append("Serialized data files: these are the files that hold all of the data for each thread of "
                   "execution in the original run.  The files are named by rank:\n"
                   "    <prefix>_<checkpoint_id>_<simulated_time>_<rank>_<thread>.bin\n\n");

        msg.append("A sample directory structure using the default name format and a checkpoint prefix of "
                   "\"checkpoint\" using two ranks with one thread each would look something like:\n\n"
                   "output directory\n"
                   "|--checkpoint\n"
                   "   |--checkpoint_1_1000\n"
                   "      |--checkpoint_1_1000.sstcpt\n"
                   "      |--checkpoint_1_1000_globals.bin\n"
                   "      |--checkpoint_1_1000_0_0.bin\n"
                   "      |--checkpoint_1_1000_1_0.bin\n"
                   "   |--checkpoint_2_2000\n"
                   "      |--checkpoint_2_2000.sstcpt\n"
                   "      |--checkpoint_2_2000_globals.bin\n"
                   "      |--checkpoint_2_2000_0_0.bin\n"
                   "      |--checkpoint_2_2000_1_0.bin\n\n");

        msg.append("When restarting from a checkpoint, the registry file (*.sstcpt) should be specified as the "
                   "input file.\n");

        return msg;
    }

    static std::string getCheckpointNameFormatExtHelp()
    {
        std::string msg = "Checkpointing Filename Formats:\n\n";
        msg.append("It is possible to set the format for the directories and filenames used for writing out "
                   "checkpoints.  The format is specified as <directory name format>/<file name format>, or you "
                   "can leave the / out and specify the same format for both directory and filenames.\n");
        msg.append("\nThe format string can contain literal text, as well as the following special control "
                   "sequences:\n\n");

        msg.append("\t%p - \vwill be replaced with the prefix specified by --checkpoint-prefix\n "
                   "\t%n - \vwill be replaced by the checkpoint index. The checkpoint index starts at 0 and "
                   "is incremented for each checkpoint that occurs.\n"
                   "\t%t - \vwill be replaced with the current simulated time at the point of the checkpoint, "
                   "expressed in sim cycles (number of core timebase units that have elapsed)\n");

        msg.append("\nThe default name format is %p_%n_%t/%p_%n_%t, or equivalently %p_%n_%t since the directory "
                   "and filename formats are the same.\n");

        msg.append("\nNOTE: The directory format should include %n and/or %t as part of the format to ensure "
                   "unique directory names for each checkpoint.  If these are not used, the same directory and "
                   "files will be used for each checkpoint and prior checkpoints will get overwritten.\n");

        msg.append("Example where prefix = \"checkpoint\", checkpoint index = 1 and time = 1000:\n\n");

        msg.append("\t%p_%n_%p   - checkpoint_1_1000\n");
        msg.append("\t%p_%n      - checkpoint_1\n");
        msg.append("\tfile_%n_%p - file_1_1000\n");

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
        if ( cfg->sigalrm_ != "" ) {
            cfg->sigalrm_ += ";";
        }
        cfg->sigalrm_ += arg;
        return 0;
    }

    // Extended help for SIGALRM
    static std::string getSignalExtHelp()
    {
        std::string msg = "RealTime Actions:\n\n";
        msg.append("  RealTimeActions are actions that execute in response to system signals SIGUSR1, SIGUSR2, and/or "
                   "SIGALRM. "
                   "The following actions are available from SST core or custom actions may also be defined.\n"
                   "   - sst.rt.exit.clean: Exits SST normally.\n"
                   "   - sst.rt.exit.emergency: Exits SST in an emergency state. Triggered on SIGINT and SIGTERM.\n"
                   "   - sst.rt.status.core: Reports brief state of SST core.\n"
                   "   - sst.rt.status.all: Reports state of SST core and every simulated component.\n"
                   "   - sst.rt.checkpoint: Creates a checkpoint.\n"
                   "   - sst.rt.heartbeat: Reports state of SST core and some profiling state (e.g., memory usage).\n"
                   "   - sst.rt.interactive: Breaks into interactive console to explore simulation state.\n"
                   "     Ignored if --interactive-console not set. (Valid for SIGUSR1/2 only, invalid for SIGALRM)\n");
        msg.append("  An action can be attached to SIGUSR1 using '--sigusr1=<handler>' and SIGUSR2 using "
                   "'--sigusr2=<handler>'\n"
                   "  If not specified SST uses the defaults: --sigusr1=sst.rt.status.core and "
                   "--sigusr2=sst.rt.status.all.\n");
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
    std::cout << "timing_json = " << timing_json_ << std::endl;
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
    std::cout << "checkpoint_name_format = " << checkpoint_name_format_ << std::endl;
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

static std::vector<AnnotationInfo> annotations = { { 'S',
    "Options annotated with 'S' can be set in the SDL file (input configuration file)\n  - Note: Options set on the "
    "command line take precedence over options set in the SDL file\n" } };

Config::Config(uint32_t num_ranks, bool first_rank) :
    ConfigShared(!first_rank, annotations)
{
    // Basic Options
    first_rank_ = first_rank;

    num_ranks_             = num_ranks;
    num_threads_           = 1;
    configFile_            = "NONE";
    model_options_         = "";
    print_timing_          = false;
    timing_json_           = "";
    stop_at_               = "0 ns";
    exit_after_            = 0;
    partitioner_           = "sst.linear";
    heartbeat_sim_period_  = "";
    heartbeat_wall_period_ = 0;

    char* wd_buf = (char*)malloc(sizeof(char) * PATH_MAX);
    (void)!getcwd(wd_buf, PATH_MAX);

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

#if PY_MINOR_VERSION >= 9
    enable_python_coverage_ = false;
#endif

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
    checkpoint_name_format_ = "%p_%n_%t/%p_%n_%t";

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
    DEF_ARG("help", 0, "option", "Print extended help information for requested option.",
        std::bind(&ConfigHelper::printHelp, this, _1), false);
    DEF_FLAG("version", 'V', "Print SST Release Version", std::bind(&ConfigHelper::printVersion, this, _1));

    /* Basic Options */
    DEF_SECTION_HEADING("Basic Options (most commonly used)");
    addVerboseOptions(true);
    DEF_ARG("num-threads", 'n', "NUM", "Number of parallel threads to use per rank",
        std::bind(&ConfigHelper::setNumThreads, this, _1), true);
    DEF_ARG("sdl-file", 0, "FILE",
        "Specify SST Configuration file.  Note: this is most often done by just specifying the file without an option.",
        std::bind(&ConfigHelper::setConfigFile, this, _1), false);
    DEF_ARG("model-options", 0, "STR",
        "Provide options to the python configuration script.  Additionally, any arguments provided after a final '-- ' "
        "will be appended to the model options (or used as the model options if --model-options was not specified).",
        std::bind(&ConfigHelper::setModelOptions, this, _1), false);
    DEF_FLAG_OPTVAL("print-timing-info", 0, "Print SST timing information",
        std::bind(&ConfigHelper::setPrintTiming, this, _1), true);
    DEF_ARG("timing-info-json", 0, "FILE", "Write SST timing information in JSON format",
        std::bind(&ConfigHelper::setTimingJSON, this, _1), true);
    DEF_ARG("stop-at", 0, "TIME", "Set time at which simulation will end execution",
        std::bind(&ConfigHelper::setStopAt, this, _1), true);
    DEF_ARG("exit-after", 0, "TIME",
        "Set the maximum wall time after which simulation will end execution.  Time is specified in hours, minutes and "
        "seconds, with the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the "
        "appropriate numbers for that value, lower case letters represent the units and are required for those "
        "formats).",
        std::bind(&ConfigHelper::setExitAfter, this, _1), true);
    DEF_ARG("partitioner", 0, "PARTITIONER", "Select the partitioner to be used. <lib.partitionerName>",
        std::bind(&ConfigHelper::setPartitioner, this, _1), true);
    DEF_ARG("heartbeat-period", 0, "PERIOD",
        "Set time for heartbeats to be published (these are approximate timings measured in simulation time, published "
        "by the core, to update on progress)",
        std::bind(&ConfigHelper::setHeartbeatSimPeriod, this, _1), true);
    DEF_ARG("heartbeat-wall-period", 0, "PERIOD",
        "Set approximate frequency for heartbeats (SST-Core progress updates) to be published in terms of wall (real) "
        "time. PERIOD can be specified in hours, minutes, and seconds with "
        "the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the appropriate numbers for "
        "that value, lower case letters represent the units and are required for those formats.).",
        std::bind(&ConfigHelper::setHeartbeatWallPeriod, this, _1), true);
    DEF_ARG("heartbeat-sim-period", 0, "PERIOD",
        "Set approximate frequency for heartbeats (SST-Core progress updates) to be published in terms of simulated "
        "time. PERIOD must include time units (s or Hz) and SI prefixes are accepted.",
        std::bind(&ConfigHelper::setHeartbeatSimPeriod, this, _1), true);
    DEF_ARG("output-directory", 0, "DIR", "Directory into which all SST output files should reside",
        std::bind(&ConfigHelper::setOutputDir, this, _1), true);
    DEF_ARG("output-prefix-core", 0, "STR", "set the SST::Output prefix for the core",
        std::bind(&ConfigHelper::setOutputPrefix, this, _1), true);

    /* Configuration Output */
    DEF_SECTION_HEADING(
        "Configuration Output Options (generates a file that can be used as input for reproducing a run)");
    DEF_ARG("output-config", 0, "FILE", "File to write SST configuration (in Python format)",
        std::bind(&ConfigHelper::setWriteConfig, this, _1), true);
    // TODO Change to output-config-json to be consistent with timing-info-json option
    DEF_ARG("output-json", 0, "FILE", "File to write SST configuration graph (in JSON format)",
        std::bind(&ConfigHelper::setWriteJSON, this, _1), true);
#ifdef SST_CONFIG_HAVE_MPI
    DEF_FLAG_OPTVAL("parallel-output", 0,
        "Enable parallel output of configuration information.  This option is ignored for single rank jobs.  Must also "
        "specify an output type (--output-config and/or --output-json).  Note: this will also cause partition info to "
        "be output if set to true.",
        std::bind(&ConfigHelper::enableParallelOutput, this, _1), true);
#endif

    /* Configuration Output */
    DEF_SECTION_HEADING("Graph Output Options (for outputting graph information for visualization or inspection)");
    DEF_ARG("output-dot", 0, "FILE", "File to write SST configuration graph (in GraphViz format)",
        std::bind(&ConfigHelper::setWriteDot, this, _1), true);
    DEF_ARG("dot-verbosity", 0, "INT", "Amount of detail to include in the dot graph output",
        std::bind(&ConfigHelper::setDotVerbosity, this, _1), true);
    DEF_ARG_OPTVAL("output-partition", 0, "FILE",
        "File to write SST component partitioning information.  When used without an argument and in conjuction with "
        "--output-json or --output-config options, will cause paritition information to be added to graph output.",
        std::bind(&ConfigHelper::setWritePartitionFile, this, _1), true);

    /* Advanced Features */
    DEF_SECTION_HEADING("Advanced Options");
    DEF_ARG_EH("timebase", 0, "TIMEBASE", "Set the base time step of the simulation (default: 1ps)",
        std::bind(&ConfigHelper::setTimebase, this, _1), std::bind(&ConfigHelper::getTimebaseExtHelp), true);
#ifdef SST_CONFIG_HAVE_MPI
    DEF_ARG_OPTVAL("parallel-load", 0, "MODE",
        "Enable parallel loading of configuration. This option is ignored for single rank jobs.  Optional mode "
        "parameters are NONE, SINGLE and MULTI (default).  If NONE is specified, parallel-load is turned off. If "
        "SINGLE is specified, the same file will be passed to all MPI ranks.  If MULTI is specified, each MPI rank is "
        "required to have it's own file to load. Note, not all input "
        "formats support both types of file loading.",
        std::bind(&ConfigHelper::enableParallelLoadMode, this, _1), false);
#endif
    DEF_ARG("timeVortex", 0, "MODULE", "Select TimeVortex implementation <lib.timevortex>",
        std::bind(&ConfigHelper::setTimeVortex, this, _1), true);
    DEF_FLAG_OPTVAL("interthread-links", 0, "[EXPERIMENTAL] Set whether or not interthread links should be used",
        std::bind(&ConfigHelper::setInterThreadLinks, this, _1), true);
#ifdef USE_MEMPOOL
    DEF_FLAG_OPTVAL("cache-align-mempools", 0, "[EXPERIMENTAL] Set whether mempool allocations are cache aligned",
        std::bind(&ConfigHelper::setCacheAlignMempools, this, _1), true);
#endif
    DEF_ARG("debug-file", 0, "FILE", "File where debug output will go",
        std::bind(&ConfigHelper::setDebugFile, this, _1), true);
    addLibraryPathOptions();

#if PY_MINOR_VERSION >= 9
    DEF_FLAG_EH("enable-python-coverage", 0,
        "[EXPERIMENTAL] Causes the base Python interpreter to activate the coverage.Coverage object. This option can "
        "also be turned "
        "on by setting the environment variable SST_CONFIG_PYTHON_COVERAGE to true.",
        std::bind(&ConfigHelper::enablePythonCoverage, this, _1), std::bind(&ConfigHelper::getPythonCoverageExtHelp),
        false);
#endif

    /* Advanced Features - Profiling */
    DEF_SECTION_HEADING("Advanced Options - Profiling (API Not Yet Final)");
    DEF_ARG_EH("enable-profiling", 0, "POINTS",
        "Enables default profiling for the specified points.  Argument is a semicolon separated list specifying the "
        "points to enable.",
        std::bind(&ConfigHelper::enableProfiling, this, _1), std::bind(&ConfigHelper::getProfilingExtHelp), true);
    DEF_ARG("profiling-output", 0, "FILE", "Set output location for profiling data [stdout (default) or a filename]",
        std::bind(&ConfigHelper::setProfilingOutput, this, _1), true);

    /* Advanced Features - Debug */
    DEF_SECTION_HEADING("Advanced Options - Debug");
    DEF_ARG("run-mode", 0, "MODE", "Set run mode [ init | run | both (default)]",
        std::bind(&ConfigHelper::setRunMode, this, _1), true);
    DEF_ARG("interactive-console", 0, "ACTION",
        "[EXPERIMENTAL] Set console to use for interactive mode. NOTE: This currently only works for serial jobs and "
        "this option will be ignored for parallel runs.",
        std::bind(&ConfigHelper::setInteractiveConsole, this, _1), true);
    DEF_ARG_OPTVAL("interactive-start", 0, "TIME",
        "[EXPERIMENTAL] Drop into interactive mode at specified simulated time.  If no time is specified, or the time "
        "is 0, then it will "
        "drop into interactive mode before any events are processed in the main run loop. This option is ignored if no "
        "interactive console was set. NOTE: This currently only works for serial jobs and this option will be ignored "
        "for parallel runs.",
        std::bind(&ConfigHelper::setInteractiveStartTime, this, _1), true);
#ifdef USE_MEMPOOL
    DEF_ARG("output-undeleted-events", 0, "FILE",
        "file to write information about all undeleted events at the end of simulation (STDOUT and STDERR can be used "
        "to output to console)",
        std::bind(&ConfigHelper::setWriteUndeleted, this, _1), true);
#endif
    DEF_FLAG("force-rank-seq-startup", 0,
        "Force startup phases of simulation to execute one rank at a time for debug purposes",
        std::bind(&ConfigHelper::forceRankSeqStartup, this, _1));

    /* Advanced Features - Environment Setup/Reporting */
    DEF_SECTION_HEADING("Advanced Options - Environment Setup/Reporting");
    addEnvironmentOptions();
    DEF_FLAG("disable-signal-handlers", 0, "Disable signal handlers",
        std::bind(&ConfigHelper::disableSigHandlers, this, _1));
    DEF_ARG_EH("sigusr1", 0, "MODULE", "Select handler for SIGUSR1 signal. See extended help for detail.",
        std::bind(&ConfigHelper::setSigUsr1, this, _1), std::bind(&ConfigHelper::getSignalExtHelp), true);
    DEF_ARG_EH("sigusr2", 0, "MODULE", "Select handler for SIGUSR2 signal. See extended help for detail.",
        std::bind(&ConfigHelper::setSigUsr2, this, _1), std::bind(&ConfigHelper::getSignalExtHelp), true);
    DEF_ARG_EH("sigalrm", 0, "MODULE",
        "Select handler for SIGALRM signals.  Argument is a semicolon separated list specifying the "
        "handlers to register along with a time interval for each. See extended help for detail.",
        std::bind(&ConfigHelper::setSigAlrm, this, _1), std::bind(&ConfigHelper::getSignalExtHelp), true);

    /* Advanced Features - Checkpoint */
    DEF_SECTION_HEADING("Advanced Options - Checkpointing (EXPERIMENTAL)");
    DEF_ARG("checkpoint-wall-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of wall (real) time. PERIOD can be "
        "specified in hours, minutes, and seconds with "
        "the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the appropriate numbers for "
        "that value, lower case letters represent the units and are required for those formats.).",
        std::bind(&ConfigHelper::setCheckpointWallPeriod, this, _1), true);
    DEF_ARG("checkpoint-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of simulated time. PERIOD must include "
        "time units (s or Hz) and SI prefixes are accepted. This flag will eventually be removed in favor of "
        "--checkpoint-sim-period",
        std::bind(&ConfigHelper::setCheckpointSimPeriod, this, _1), true);
    DEF_ARG("checkpoint-sim-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of simulated time. PERIOD must include "
        "time units (s or Hz) and SI prefixes are accepted.",
        std::bind(&ConfigHelper::setCheckpointSimPeriod, this, _1), true);
    DEF_FLAG("load-checkpoint", 0,
        "Load checkpoint and continue simulation. Specified SDL file will be used as the checkpoint file.",
        std::bind(&ConfigHelper::setLoadFromCheckpoint, this, _1), false);
    DEF_ARG_EH("checkpoint-prefix", 0, "PREFIX",
        "Set prefix for checkpoint filenames. The checkpoint prefix defaults to checkpoint if this option is not set "
        "and checkpointing is enabled.",
        std::bind(&ConfigHelper::setCheckpointPrefix, this, _1), std::bind(&ConfigHelper::getCheckpointPrefixExtHelp),
        true);
    DEF_ARG_EH("checkpoint-name-format", 0, "FORMAT",
        "Set the format for checkpoint filenames. See extended help for format options.  Default is "
        "\"%p_%n_%t/%p_%n_%t\"",
        std::bind(&ConfigHelper::setCheckpointNameFormat, this, _1),
        std::bind(&ConfigHelper::getCheckpointNameFormatExtHelp), true);

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
        fprintf(stderr, "Error: sdl-file name is the only positional argument allowed.  See help for --model-options "
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

    return 0;
}


bool
Config::setOptionFromModel(const std::string& entryName, const std::string& value)
{
    // Check to make sure option is settable in the SDL file
    if ( getAnnotation(entryName, 'S') ) {
        return setOptionExternal(entryName, value);
    }
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
