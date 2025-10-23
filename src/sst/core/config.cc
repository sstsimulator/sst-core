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
#include "sst/core/stringize.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/warnmacros.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace SST {

namespace StandardConfigParsers {

int
check_unitalgebra_store_string(std::string valid_units, std::string& var, std::string arg)
{
    // Parse the valid units
    if ( arg.empty() ) return 0;

    std::vector<std::string> units;
    tokenize(units, valid_units, ",", true);

    try {
        UnitAlgebra check(arg);

        // Check for valid units
        bool valid = false;
        for ( auto x : units ) {
            if ( check.hasUnits(x) ) valid = true;
        }

        if ( !valid ) {
            fprintf(stderr, "Error parsing option: Units passed to %s must be one of: %s. Argument = [%s]\n",
                ConfigBase::currently_parsing_option.c_str(), valid_units.c_str(), arg.c_str());
            return -1;
        }
    }
    catch ( UnitAlgebra::InvalidUnitType const& ) {
        fprintf(stderr, "Error parsing option: Invalid units passed to %s. Argument = [%s]\n",
            ConfigBase::currently_parsing_option.c_str(), arg.c_str());
        return -1;
    }
    catch ( ... ) {
        fprintf(stderr, "Error parsing option: Argument passed to %s cannot be parsed. Argument = [%s]\n",
            ConfigBase::currently_parsing_option.c_str(), arg.c_str());
        return -1;
    }
    var = arg;
    return 0;
}


} // namespace StandardConfigParsers


/**** Extended Help Functions ****/
std::string
Config::ext_help_timebase()
{
    std::string msg = "Timebase:\n\n";
    msg.append("Time in SST core is represented by a 64-bit unsigned integer.  By default, each count of that "
               "value represents 1ps of time.  The timebase option allows you to set that atomic core timebase to "
               "a different value.\n ");
    msg.append("There are two things to balance when determining a timebase to use:\n\n");
    msg.append("1) The shortest time period or fastest clock frequency you want to represent:\n");
    msg.append("  It is recommended that the core timebase be set to ~1000x smaller than the shortest time period "
               "(fastest frequency) in your simulation.  For the default 1ps timebase, clocks in the 1-10GHz range "
               "are easily represented.  If you want to have higher frequency clocks, you can set the timebase to "
               "a smaller value, at the cost of decreasing the amount of time you can simulate.\n\n");
    msg.append("2) How much simulated time you need to support:\n");
    msg.append("  The default timebase of 1ps will support ~215.5 days (5124 hours) of simulated time.  If you are "
               "using SST to simulate longer term phenomena, you will need to make the core timebase longer.  A "
               "consequence of increasing the timebase is that the minimum time period that can be represented will "
               "increase (conversely, the maximum frequency that can be represented will increase).");
    return msg;
}

#if PY_MINOR_VERSION >= 9
std::string
Config::ext_help_enable_python_coverage()
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

std::string
Config::ext_help_enable_profiling()
{
    std::string msg = "Profiling Points [API Not Yet Final]:\n\n";
    msg.append("NOTE: Profiling points are still in development and syntax for enabling profiling tools is subject to "
               "change. Additional profiling points may also be added in the future.\n\n");
    msg.append("Profiling points are points in the code where a profiling tool can be instantiated.  The "
               "profiling tool allows you to collect various data about code segments.  There are currently three "
               "profiling points in SST core:\n\n");
    msg.append("\tclock: \t\vprofiles calls to user registered clock handlers\n");
    msg.append("\tevent: \t\vprofiles calls to user registered event handlers set on Links\n");
    msg.append("\tsync:  \t\vprofiles calls into the SyncManager (only valid for parallel simulations)\n");
    msg.append("\n");
    msg.append("The format for enabling a profile point is a semicolon separated list where each item specifies "
               "details for a given profiling tool using the following format:\n\n");
    msg.append("\tname:type(params)[point], where\n");
    msg.append("\t\tname   \t= \vname of tool to be shown in output\n");
    msg.append("\t\ttype   \t= \vtype of profiling tool in ELI format (lib.type)\n");
    msg.append("\t\tparams \t= \voptional parameters to pass to profiling tool, format is key=value,key=value...\n");
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

// Extended help for SIGALRM
std::string
Config::ext_help_signals()
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

std::string
Config::ext_help_checkpoint_prefix()
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

std::string
Config::ext_help_checkpoint_format()
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


//// Config Class functions

void
Config::print()
{
    std::vector<std::string> kv_pairs;
    for ( auto& option : options ) {
        if ( option.header ) continue;
        option.def->to_string(kv_pairs);
    }

    for ( auto& str : kv_pairs ) {
        printf("%s\n", str.c_str());
    }
}

void
Config::serialize_order(SST::Core::Serialization::serializer& ser)
{
    for ( auto& option : options ) {
        if ( !option.header ) {
            option.def->serialize(ser);
        }
    }
}

void
Config::merge_checkpoint_options(Config& other)
{
    // Need to get the indices for R and X annotations (these can
    // change if more annotations are added or deleted)
    size_t r_index = getAnnotationIndex('R');
    size_t x_index = getAnnotationIndex('X');

    for ( size_t i = 0; i < options.size(); ++i ) {
        auto& option = options[i];
        if ( option.header ) continue;

        // Check to see if the current option carries over through a
        // checkpoint/restart. If not continue to next option
        if ( option.annotations.size() <= r_index || !option.annotations[r_index] ) continue;

        // Now see if we need to copy over the option.  We copy it if
        // it wasn't set on the command line, or it isn't allowed to
        // be set on the command line for a restart.
        if ( !option.def->set_cmdline || option.annotations[x_index] ) {
            option.def->transfer(other.options[i].def);
        }
    }
}


Config::Config() :
    ConfigShared()
{
    // Basic Options

    // Set output_directory_ to current working directory
    char* wd_buf = (char*)malloc(sizeof(char) * PATH_MAX);
    (void)!getcwd(wd_buf, PATH_MAX);

    output_directory_.value = "";
    if ( nullptr != wd_buf ) {
        output_directory_.value.append(wd_buf);
        free(wd_buf);
    }

    // Add annotations
    addAnnotation(
        { 'S', "Options annotated with 'S' can be set in the SDL file (input configuration file)\n"
               "  - Note: Options set on the command line take precedence over options set in the SDL file" });
    addAnnotation({ 'R',
        "Options annotated with 'R' will be carried through a checkpoint to a restart run\n"
        "  - These options can be overwritten on the command line of a restart run, unless annotated with 'X'" });
    addAnnotation({ 'X', "Options annotated with 'X' are ignored for restart runs" });

    insertOptions();
}

void
Config::initialize(uint32_t num_ranks, bool first_rank)
{
    num_ranks_ = num_ranks;
    if ( first_rank ) enable_printing();
}


// Check for existence of config file
bool
Config::checkConfigFile()
{
    struct stat sb;
    char*       fqpath = realpath(configFile_.value.c_str(), nullptr);
    if ( nullptr == fqpath ) {
        fprintf(stderr, "Failed to canonicalize path [%s]:  %s\n", configFile_.value.c_str(), strerror(errno));
        return false;
    }
    configFile_ = fqpath;
    free(fqpath);
    if ( 0 != stat(configFile_.value.c_str(), &sb) ) {
        fprintf(stderr, "File [%s] cannot be found: %s\n", configFile_.value.c_str(), strerror(errno));
        return false;
    }
    if ( !S_ISREG(sb.st_mode) ) {
        fprintf(stderr, "File [%s] is not a regular file.\n", configFile_.value.c_str());
        return false;
    }

    if ( 0 != access(configFile_.value.c_str(), R_OK) ) {
        fprintf(stderr, "File [%s] is not readable.\n", configFile_.value.c_str());
        return false;
    }

    return true;
}

void
Config::insertOptions()
{
    using namespace std::placeholders;
    // Informational options
    DEF_SECTION_HEADING("Informational Options");
    DEF_FLAG("usage", 'h', "Print usage information.", usage_);
    DEF_ARG("help", 0, "option", "Print extended help information for requested option.", help_, false);
    DEF_FLAG("version", 'V', "Print SST Release Version", version_);

    /* Basic Options */
    DEF_SECTION_HEADING("Basic Options (most commonly used)");
    addVerboseOptions(true);
    DEF_ARG("num-threads", 'n', "NUM", "Number of parallel threads to use per rank", num_threads_, true);
    DEF_ARG("sdl-file", 0, "FILE",
        "Specify SST Configuration file.  Note: this is most often done by just specifying the file without an option. "
        "The SDL file can be the manifest file from a checkpoint (*.sstcpt).",
        configFile_, false);
    DEF_ARG("model-options", 0, "STR",
        "Provide options to the python configuration script.  Additionally, any arguments provided after a final '-- ' "
        "will be appended to the model options (or used as the model options if --model-options was not specified).",
        model_options_, false, false, true);
    DEF_ARG_OPTVAL("print-timing-info", 0, "LEVEL",
        "Print SST timing information.  Can supply an optional level to control the granularity of timing information. "
        "Level = 0 turns all timing info off, level = 1 will print total runtime, as well as other performance data. "
        "Level >= 2 will print increasing granularity of performance data. If specified with no level, then the level "
        "will be set to 2.",
        print_timing_, true, true, false);
    DEF_ARG(
        "timing-info-json", 0, "FILE", "Write SST timing information in JSON format", timing_json_, true, true, false);
    DEF_ARG("stop-at", 0, "TIME", "Set time at which simulation will end execution", stop_at_, true, true, false);
    DEF_ARG("exit-after", 0, "TIME",
        "Set the maximum wall time after which simulation will end execution.  Time is specified in hours, minutes and "
        "seconds, with the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the "
        "appropriate numbers for that value, lower case letters represent the units and are required for those "
        "formats).",
        exit_after_, true, false, false);
    DEF_ARG("partitioner", 0, "PARTITIONER", "Select the partitioner to be used. <lib.partitionerName>", partitioner_,
        true, true, false);
    DEF_ARG("heartbeat-period", 0, "PERIOD",
        "Set time for heartbeats to be published (these are approximate timings measured in simulation time, published "
        "by the core, to update on progress)",
        heartbeat_sim_period_, true, true, false);
    DEF_ARG("heartbeat-wall-period", 0, "PERIOD",
        "Set approximate frequency for heartbeats (SST-Core progress updates) to be published in terms of wall (real) "
        "time. PERIOD can be specified in hours, minutes, and seconds with "
        "the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the appropriate numbers for "
        "that value, lower case letters represent the units and are required for those formats.).",
        heartbeat_wall_period_, true, true, false);
    DEF_ARG("heartbeat-sim-period", 0, "PERIOD",
        "Set approximate frequency for heartbeats (SST-Core progress updates) to be published in terms of simulated "
        "time. PERIOD must include time units (s or Hz) and SI prefixes are accepted.",
        heartbeat_sim_period_, true, true, false);
    DEF_ARG("output-directory", 0, "DIR", "Directory into which all SST output files should reside", output_directory_,
        true, false, false);
    DEF_ARG("output-prefix-core", 0, "STR", "set the SST::Output prefix for the core", output_core_prefix_, true, true,
        true);

    /* Configuration Output */
    DEF_SECTION_HEADING(
        "Configuration Output Options (generates a file that can be used as input for reproducing a run)");
    DEF_ARG("output-config", 0, "FILE", "File to write SST configuration (in Python format)", output_config_graph_,
        true, false, true);
    DEF_ARG("output-json", 0, "FILE", "File to write SST configuration graph (in JSON format)", output_json_, true,
        false, true);
#ifdef SST_CONFIG_HAVE_MPI
    DEF_FLAG_OPTVAL("parallel-output", 0,
        "Enable parallel output of configuration information.  This option is ignored for single rank jobs.  Must also "
        "specify an output type (--output-config and/or --output-json).  Note: this will also cause partition info to "
        "be output if set to true.",
        parallel_output_, true, false, true);
#endif

    /* Configuration Output */
    DEF_SECTION_HEADING("Graph Output Options (for outputting graph information for visualization or inspection)");
    DEF_ARG("output-dot", 0, "FILE", "File to write SST configuration graph (in GraphViz format)", output_dot_, true,
        false, true);
    DEF_ARG("dot-verbosity", 0, "INT", "Amount of detail to include in the dot graph output", dot_verbosity_, true,
        false, true);
    DEF_ARG_OPTVAL("output-partition", 0, "FILE",
        "File to write SST component partitioning information.  When used without an argument and in conjunction with "
        "--output-json or --output-config options, will cause partition information to be added to graph output.",
        output_partition_, true, false, false);

    /* Advanced Features */
    DEF_SECTION_HEADING("Advanced Options");
    DEF_ARG("timebase", 0, "TIMEBASE", "Set the base time step of the simulation (default: 1ps)", timeBase_, true, true,
        true);
#ifdef SST_CONFIG_HAVE_MPI
    DEF_ARG_OPTVAL("parallel-load", 0, "MODE",
        "Enable parallel loading of configuration. This option is ignored for single rank jobs.  Optional mode "
        "parameters are NONE, SINGLE and MULTI (default).  If NONE is specified, parallel-load is turned off. If "
        "SINGLE is specified, the same file will be passed to all MPI ranks.  If MULTI is specified, each MPI rank is "
        "required to have it's own file to load. Note, not all input "
        "formats support both types of file loading.",
        parallel_load_, false, false, true);
#endif
    DEF_ARG(
        "timeVortex", 0, "MODULE", "Select TimeVortex implementation <lib.timevortex>", timeVortex_, true, true, false);
    DEF_FLAG_OPTVAL("interthread-links", 0, "[EXPERIMENTAL] Set whether or not interthread links should be used",
        interthread_links_, true);
#ifdef USE_MEMPOOL
    DEF_FLAG_OPTVAL("cache-align-mempools", 0, "[EXPERIMENTAL] Set whether mempool allocations are cache aligned",
        cache_align_mempools_, true, true, true);
#endif
    DEF_ARG("debug-file", 0, "FILE", "File where debug output will go", debugFile_, true, false, true);
    addLibraryPathOptions();

#if PY_MINOR_VERSION >= 9
    DEF_FLAG("enable-python-coverage", 0,
        "[EXPERIMENTAL] Causes the base Python interpreter to activate the coverage.Coverage object. This option can "
        "also be turned "
        "on by setting the environment variable SST_CONFIG_PYTHON_COVERAGE to true.",
        enable_python_coverage_, false, false, true);
#endif

    /* Advanced Features - Profiling */
    DEF_SECTION_HEADING("Advanced Options - Profiling (API Not Yet Final)");
    DEF_ARG("enable-profiling", 0, "POINTS",
        "Enables default profiling for the specified points.  Argument is a semicolon separated list specifying the "
        "points to enable.",
        enabled_profiling_, true, false, false);
    DEF_ARG("profiling-output", 0, "FILE", "Set output location for profiling data [stdout (default) or a filename]",
        profiling_output_, true, false, false);

    /* Advanced Features - Debug */
    DEF_SECTION_HEADING("Advanced Options - Debug");
    DEF_ARG("run-mode", 0, "MODE", "Set run mode [ init | run | both (default)]", runMode_, true, false, true);
    DEF_ARG("interactive-console", 0, "ACTION",
        "[EXPERIMENTAL] Set console to use for interactive mode (overrides default console: "
        "sst.interactive.simpledebug). "
        "NOTE: This currently only works for serial jobs and will be ignored for parallel runs.",
        interactive_console_, true, false, false);
    DEF_ARG_OPTVAL("interactive-start", 0, "TIME",
        "[EXPERIMENTAL] Drop into interactive mode at specified simulated time.  If no time is specified, or the time "
        "is 0, then it will drop into interactive mode before any events are processed in the main run loop. "
        "NOTE: This currently only works for serial jobs and this option will be ignored "
        "for parallel runs.",
        interactive_start_time_, true, false, false);
    DEF_ARG("replay-file", 0, "FILE", "Specify file for replaying an interactive debug console session.", replay_file_,
        false);
#ifdef USE_MEMPOOL
    DEF_ARG("output-undeleted-events", 0, "FILE",
        "file to write information about all undeleted events at the end of simulation (STDOUT and STDERR can be used "
        "to output to console)",
        event_dump_file_, true, false, false);
#endif
    DEF_FLAG("force-rank-seq-startup", 0,
        "Force startup phases of simulation to execute one rank at a time for debug purposes", rank_seq_startup_, false,
        false, true);

    /* Advanced Features - Environment Setup/Reporting */
    DEF_SECTION_HEADING("Advanced Options - Environment Setup/Reporting");
    addEnvironmentOptions();
    DEF_FLAG("disable-signal-handlers", 0, "Disable signal handlers", enable_sig_handling_, false, false, false);
    DEF_ARG("sigusr1", 0, "MODULE", "Select handler for SIGUSR1 signal. See extended help for detail.", sigusr1_, true,
        true, false);
    DEF_ARG("sigusr2", 0, "MODULE", "Select handler for SIGUSR2 signal. See extended help for detail.", sigusr2_, true,
        true, false);
    DEF_ARG("sigalrm", 0, "MODULE",
        "Select handler for SIGALRM signals.  Argument is a semicolon separated list specifying the "
        "handlers to register along with a time interval for each. See extended help for detail.",
        sigalrm_, true, true, false);

    /* Advanced Features - Checkpoint */
    DEF_SECTION_HEADING("Advanced Options - Checkpointing (EXPERIMENTAL)");
    DEF_FLAG("checkpoint-enable", 0,
        "Allows checkpoints to be triggered from the interactive debug console. "
        "This option is not needed if checkpoint-wall-period, checkpoint-period, or checkpoint-sim-period are used.",
        checkpoint_enable_, false, false, false);
    DEF_ARG("checkpoint-wall-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of wall (real) time. PERIOD can be "
        "specified in hours, minutes, and seconds with "
        "the following formats supported: H:M:S, M:S, S, Hh, Mm, Ss (capital letters are the appropriate numbers for "
        "that value, lower case letters represent the units and are required for those formats.).",
        checkpoint_wall_period_, true, false, false);
    DEF_ARG("checkpoint-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of simulated time. PERIOD must include "
        "time units (s or Hz) and SI prefixes are accepted. This flag will eventually be removed in favor of "
        "--checkpoint-sim-period",
        checkpoint_sim_period_, true, false, false);
    DEF_ARG("checkpoint-sim-period", 0, "PERIOD",
        "Set approximate frequency for checkpoints to be generated in terms of simulated time. PERIOD must include "
        "time units (s or Hz) and SI prefixes are accepted.",
        checkpoint_sim_period_, true, false, false);
    DEF_FLAG("load-checkpoint", 0,
        "[UNUSED] This options is no longer needed.  SST will automatically detect if a checkpoint file is specified "
        "as the SDL file by detecting the .sstcpt extension.",
        load_from_checkpoint_, true, false, false);
    DEF_ARG("checkpoint-prefix", 0, "PREFIX",
        "Set prefix for checkpoint filenames. The checkpoint prefix defaults to checkpoint if this option is not set "
        "and checkpointing is enabled.",
        checkpoint_prefix_, true);
    DEF_ARG("checkpoint-name-format", 0, "FORMAT",
        "Set the format for checkpoint filenames. See extended help for format options.  Default is "
        "\"%p_%n_%t/%p_%n_%t\"",
        checkpoint_name_format_, true, false, false);

    enableDashDashSupport(std::bind(&OptionDefinition::parse, &model_options_, _1));
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
    if ( configFile_.value == "NONE" ) {
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
    if ( checkpoint_enable_.value == true ) return true;
    if ( checkpoint_wall_period_.value != 0 ) return true;
    if ( checkpoint_sim_period_.value != "" ) return true;
    return false;
}

// Set the prefix for checkpoint files
int
Config::parse_checkpoint_name_format(std::string& var, std::string arg)
{
    if ( arg == "" ) {
        fprintf(stderr, "Error, checkpoint-name-format must not be an empty string\n");
        return -1;
    }
    var = arg;

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


} // namespace SST
