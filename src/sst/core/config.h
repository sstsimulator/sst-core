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

#ifndef SST_CORE_CONFIG_H
#define SST_CORE_CONFIG_H

#include "sst/core/configShared.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"

// Pull in the patchlevel for Python so we can check Python version
#include <patchlevel.h>
#include <string>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);


namespace SST {
class Config;

class SSTModelDescription;
class UnitAlgebra;

namespace StandardConfigParsers {

int check_unitalgebra_store_string(std::string valid_units, std::string& var, std::string arg);

} // namespace StandardConfigParsers


/**
 * Class to contain SST Simulation Configuration variables.
 *
 * NOTE: This class needs to be serialized for the sst.x executable,
 * but not for the sst (wrapper compiled from the boot*.{h,cc} files)
 * executable.  To avoid having to compile all the serialization code
 * into the bootstrap executable, the Config class is the first level
 * of class hierarchy to inherit from serializable.
 */
class Config : public ConfigShared, public SST::Core::Serialization::serializable
{

public:
    // Main creates the config object
    friend int ::main(int argc, char** argv);
    friend class SSTModelDescription;
    friend class Simulation_impl;

    /**
       Default constructor.
     */
    Config();

private:
    /**
       Initial Config object created with default constructor
     */
    void initialize(uint32_t num_ranks, bool first_rank);

    //// Functions for use in main

    /**
       Checks for the existance of the config file.  This needs to be
       called after adding any rank numbers to the file in the case of
       parallel loading.
    */
    bool checkConfigFile();

    // Function to be called from ModelDescriptions

    /** Set a configuration string to update configuration values */
    bool setOptionFromModel(const std::string& entryName, const std::string& value);


public:
    ~Config() {}

    // Functions to access config options. Declared in order they show
    // up in the options array

    // Information options
    // No variable associated with help
    // No variable associated with version

    // Basic options

    /**
       Level of verbosity to use in the core prints using
       Output.verbose or Output.debug.

       ** Included in ConfigShared
       uint32_t verbose() const { return verbose_; }
    */

    /********************************************************************
       Define all the options in the Config object

       This section uses the SST_CONFIG_DECLARE_OPTION macro that
       defines both the variable (name_) and the getter function
       (name()).

       The options should be in the same order as they appear in the
       insertOptions() function.
    ********************************************************************/

    /**** Informational Options ****/

    /**
      Print the usage text
    */
    int parseUsage(std::string UNUSED(arg)) { return printUsage(); }

    SST_CONFIG_DECLARE_OPTION_NOVAR(usage, std::bind(&Config::parseUsage, this, std::placeholders::_1));

    /**
       Print extended help
     */
    int parseHelp(std::string arg)
    {
        if ( !arg.empty() ) return printExtHelp(arg);
        return printUsage();
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(help, std::bind(&Config::parseHelp, this, std::placeholders::_1));

    /**
       Print version info
     */
    static int parseVersion(std::string UNUSED(arg))
    {
        printf("SST-Core Version (" PACKAGE_VERSION);
        if ( strcmp(SSTCORE_GIT_HEADSHA, PACKAGE_VERSION) ) {
            printf(", git branch : " SSTCORE_GIT_BRANCH);
            printf(", SHA: " SSTCORE_GIT_HEADSHA);
        }
        printf(")\n");

        return 1; /* Should not continue, but clean exit */
    }

    SST_CONFIG_DECLARE_OPTION_NOVAR(version, std::bind(&Config::parseVersion, std::placeholders::_1));

private:

    /**
       Number of ranks in the simulation
     */
    uint32_t num_ranks_ = 1;

public:
    uint32_t num_ranks() const { return num_ranks_; }

private:

    /**** Basic Options ****/

    /**
       Number of threads requested
    */
    SST_CONFIG_DECLARE_OPTION(uint32_t, num_threads, 1, &StandardConfigParsers::from_string<uint32_t>);

    /**
       Name of the SDL file to use to generate the simulation
    */
    SST_CONFIG_DECLARE_OPTION(std::string, configFile, "NONE", &StandardConfigParsers::from_string<std::string>);

    /**
       Model options to pass to the SDL file
    */
    SST_CONFIG_DECLARE_OPTION(std::string, model_options, "",
        std::bind(&StandardConfigParsers::append_string, " \"", "\"", std::placeholders::_1, std::placeholders::_2));

    /**
       Print SST timing information after the run
    */
    SST_CONFIG_DECLARE_OPTION(int, print_timing, 0,
        std::bind(&StandardConfigParsers::from_string_default<int>, std::placeholders::_1, std::placeholders::_2, 2));

    /**
        Print SST timing information to JSON file
    */
    SST_CONFIG_DECLARE_OPTION(std::string, timing_json, "", &StandardConfigParsers::from_string<std::string>);

    /**
       Simulated cycle to stop the simulation at
    */
    SST_CONFIG_DECLARE_OPTION(std::string, stop_at, "0ns", &StandardConfigParsers::from_string<std::string>);

    /**
       Wall clock time (approximiate) in seconds to stop the simulation at
    */
    SST_CONFIG_DECLARE_OPTION(uint32_t, exit_after, 0, &StandardConfigParsers::wall_time_to_seconds);

    /**
       Partitioner to use for parallel simulations
    */
    SST_CONFIG_DECLARE_OPTION(std::string, partitioner, "sst.linear", &StandardConfigParsers::element_name);

    /**
       Wall-clock period at which to print out a "heartbeat" message
    */
    SST_CONFIG_DECLARE_OPTION(uint32_t, heartbeat_wall_period, 0, &StandardConfigParsers::wall_time_to_seconds);

    /**
       Simulation period at which to print out a "heartbeat" message
    */
    SST_CONFIG_DECLARE_OPTION(std::string, heartbeat_sim_period, "",
        std::bind(&StandardConfigParsers::check_unitalgebra_store_string, "s, Hz", std::placeholders::_1,
            std::placeholders::_2));

    /**
       The directory to be used for writing output files
    */
    SST_CONFIG_DECLARE_OPTION(std::string, output_directory, "", &StandardConfigParsers::from_string<std::string>);

    /**
       Prefix to use for the default SST::Output object in core
    */
    SST_CONFIG_DECLARE_OPTION(
        std::string, output_core_prefix, "@x SST Core: ", &StandardConfigParsers::from_string<std::string>);


    /**** Configuration output ****/

    /**
       File to output python formatted  config graph to (empty string means no
       output)
    */
    SST_CONFIG_DECLARE_OPTION(std::string, output_config_graph, "", &StandardConfigParsers::from_string<std::string>);

    /**
       File to output json formatted config graph to (empty string means no
       output)
    */
    SST_CONFIG_DECLARE_OPTION(std::string, output_json, "", &StandardConfigParsers::from_string<std::string>);

    /**
       If true, and a config graph output option is specified, write
       each ranks graph separately
    */
    int parse_parallel_output(bool& var, std::string arg)
    {
        if ( num_ranks_ == 1 ) return 0;

        int ret_code = StandardConfigParsers::flag_default_true(var, arg);
        if ( ret_code != 0 ) return ret_code;

        // If parallel_output_ (var) is true, we also need output_partition_
        // to be true.
        if ( var ) output_partition_.value1 = true;
        return 0;
    }

    SST_CONFIG_DECLARE_OPTION(bool, parallel_output, false,
        std::bind(&Config::parse_parallel_output, this, std::placeholders::_1, std::placeholders::_2));


    /**** Graph output ****/

    /**
       File to output dot formatted config graph to (empty string means no
       output).  Note, this is not a format that can be used as input for simulation

    */
    SST_CONFIG_DECLARE_OPTION(std::string, output_dot, "", &StandardConfigParsers::from_string<std::string>);

    /**
       Level of verbosity to use for the dot output.
    */
    SST_CONFIG_DECLARE_OPTION(uint32_t, dot_verbosity, 0, &StandardConfigParsers::from_string<uint32_t>);

    /**
       Controls whether partition info is output as part of configuration output
     */
    int parse_output_partition(bool& output_part_flag, std::string& file_name, std::string arg)
    {
        if ( arg == "" ) {
            output_part_flag = true;
        }
        else {
            file_name = arg;
        }
        return 0;
    }

    SST_CONFIG_DECLARE_OPTION_PAIR(bool, output_partition, false, std::string, component_partition_file, "",
        std::bind(&Config::parse_output_partition, this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
    /**
       File to output component partition info to (empty string means no output)
    */
    // DEC_OPTION(component_partition_file, "");

    /**** Advanced options ****/

    /**
       Core timebase to use as the atomic time unit for the
       simulation.  It is usually best to just leave this at the
       default (1ps)
    */
    static std::string ext_help_timebase();

    SST_CONFIG_DECLARE_OPTION(std::string, timeBase, "1ps",
        std::bind(&StandardConfigParsers::check_unitalgebra_store_string, "s, Hz", std::placeholders::_1,
            std::placeholders::_2),
        &Config::ext_help_timebase);


    /**
       Parallel loading

       parallel_load - Controls whether graph constuction will be done
       in parallel.  If it is, then the SDL file name is modified to
       add the rank number to the file name right before the file
       extension, if parallel_load_mode_multi is true.

       parallel-load-mode-multi - If graph constuction will be done in
       parallel, will use a file per rank if true, and the same file
       for each rank if false.
    */

    /**
       Returns the string equivalent for parallel-load: NONE (if
       parallel load is off), SINGLE or MULTI.
    */

    int parse_parallel_load(bool& parallel_load, bool& parallel_load_mode_multi, std::string arg)
    {
        // If this is only a one rank job, then we can ignore
        if ( num_ranks_ == 1 ) return 0;

        if ( arg == "" ) {
            parallel_load = true;
            return 0;
        }

        std::string arg_lower(arg);
        std::locale loc;
        for ( auto& ch : arg_lower )
            ch = std::tolower(ch, loc);

        if ( arg_lower == "none" ) {
            parallel_load = false;
            return 0;
        }
        else
            parallel_load = true;

        if ( arg_lower == "single" )
            parallel_load_mode_multi = false;
        else if ( arg_lower == "multi" )
            parallel_load_mode_multi = true;
        else {
            fprintf(stderr,
                "Invalid option '%s' passed to --parallel-load.  Valid options are NONE, SINGLE and MULTI.\n",
                arg.c_str());
            return -1;
        }
        return 0;
    }

public:
    std::string parallel_load_str() const
    {
        if ( !parallel_load_.value1 ) return "NONE";
        if ( parallel_load_.value2 ) return "MULTI";
        return "SINGLE";
    }

private:

    SST_CONFIG_DECLARE_OPTION_PAIR(bool, parallel_load, false, bool, parallel_load_mode_multi, true,
        std::bind(
            &Config::parse_parallel_load, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    /**
       TimeVortex implementation to use
    */
    SST_CONFIG_DECLARE_OPTION(
        std::string, timeVortex, "sst.timevortex.priority_queue", &StandardConfigParsers::from_string<std::string>);

    /**
       Use links that connect directly to ActivityQueue in receiving thread
    */
    SST_CONFIG_DECLARE_OPTION(bool, interthread_links, false, &StandardConfigParsers::flag_default_true);


#ifdef USE_MEMPOOL
    /**
       Controls whether mempool items are cache-aligned

    */
    SST_CONFIG_DECLARE_OPTION(bool, cache_align_mempools, false, &StandardConfigParsers::flag_default_true);
#endif
    /**
       File to which core debug information should be written
    */
    SST_CONFIG_DECLARE_OPTION(std::string, debugFile, "/dev/null", &StandardConfigParsers::from_string<std::string>);

    /**
       Library path to use for finding element libraries (will replace
       the libpath in the sstsimulator.conf file)

       ** Included in ConfigShared
       const std::string& libpath() const { return libpath_; }
    */

    /**
       Paths to add to library search (adds to libpath found in
       sstsimulator.conf file)

       ** Included in ConfigShared
       const std::string& addLibPath() const { return addLibPath_; }
    */


#if PY_MINOR_VERSION >= 9
    /**
       Controls whether the Python coverage object will be loaded
     */
    static std::string ext_help_enable_python_coverage();

    SST_CONFIG_DECLARE_OPTION(bool, enable_python_coverage, false, &StandardConfigParsers::flag_set_true,
        &Config::ext_help_enable_python_coverage);
#endif


    /**** Advanced options - Profiling ****/

    /**
       Profiling points to turn on
     */
    static std::string ext_help_enable_profiling();

    SST_CONFIG_DECLARE_OPTION(std::string, enabled_profiling, "",
        std::bind(&StandardConfigParsers::append_string, ";", "", std::placeholders::_1, std::placeholders::_2),
        &Config::ext_help_enable_profiling);

    /**
       Profiling points to turn on
     */
    SST_CONFIG_DECLARE_OPTION(
        std::string, profiling_output, "stdout", &StandardConfigParsers::from_string<std::string>);


    /**** Advanced options - Debug ****/

    /**
       Run mode to use (Init, Both, Run-only).  Note that Run-only is
       not currently supported because there is not component level
       checkpointing.
    */
    static int parseRunMode(SimulationRunMode& val, std::string arg)
    {
        if ( !arg.compare("init") )
            val = SimulationRunMode::INIT;
        else if ( !arg.compare("run") )
            val = SimulationRunMode::RUN;
        else if ( !arg.compare("both") )
            val = SimulationRunMode::BOTH;
        else {
            fprintf(stderr, "Unknown option for --run-mode: %s\n", arg.c_str());
            val = SimulationRunMode::UNKNOWN;
        }

        return val != SimulationRunMode::UNKNOWN ? 0 : -1;
    }

    SST_CONFIG_DECLARE_OPTION(SimulationRunMode, runMode, SimulationRunMode::BOTH, &Config::parseRunMode);

public:
    /**
       Get string version of runmode.
    */
    std::string runMode_str() const
    {
        switch ( runMode_ ) {
        case SimulationRunMode::INIT:
            return "INIT";
        case SimulationRunMode::RUN:
            return "RUN";
        case SimulationRunMode::BOTH:
            return "BOTH";
        case SimulationRunMode::UNKNOWN:
            return "UNKNOWN";
        }
        return "UNKNOWN";
    }

private:

    /**
       Get the InteractiveAction to use for interactive mode
     */
    SST_CONFIG_DECLARE_OPTION(std::string, interactive_console, "", &StandardConfigParsers::from_string<std::string>);

    /**
       Get the time to start interactive mode
    */
    SST_CONFIG_DECLARE_OPTION(std::string, interactive_start_time, "",
        std::bind(&StandardConfigParsers::from_string_default<std::string>, std::placeholders::_1,
            std::placeholders::_2, "0"));

    /**
       File to replay an interactive console script
    */
    SST_CONFIG_DECLARE_OPTION(std::string, replay_file, "", &StandardConfigParsers::from_string<std::string>);

#ifdef USE_MEMPOOL
    /**
       File to output list of events that remain undeleted at the end
       of the simulation.

       If no mempools, just reutrn empty string.  This avoids a check
    for mempools in main.cc
    */
    SST_CONFIG_DECLARE_OPTION(std::string, event_dump_file, "", &StandardConfigParsers::from_string<std::string>);
#endif

    /**
       Run simulation initialization stages one rank at a time for
       debug purposes
     */
    SST_CONFIG_DECLARE_OPTION(bool, rank_seq_startup, false, &StandardConfigParsers::flag_set_true);


    /**** Advanced options - envrionment ****/

    /**
       Controls whether the environment variables that SST sees are
       printed out

       ** Included in ConfigShared
       bool print_env() const { return print_env_; }
    */

    /**
        ** Included in ConfigShared
        bool no_env_config() const { return no_env_config_; }
    */

    /**
       Controls whether signal handlers are enable or not.  NOTE: the
       sense of this variable is opposite of the command1 line option
       (--disable-signal-handlers)
    */
    SST_CONFIG_DECLARE_OPTION(bool, enable_sig_handling, true, &StandardConfigParsers::flag_set_false);

    /**
     * SIGUSR1 handler
     */
    static std::string ext_help_signals();

    SST_CONFIG_DECLARE_OPTION(std::string, sigusr1, "sst.rt.status.core",
        &StandardConfigParsers::from_string<std::string>, Config::ext_help_signals);

    /**
     * SIGUSR2 handler
     */
    SST_CONFIG_DECLARE_OPTION(std::string, sigusr2, "sst.rt.status.all",
        &StandardConfigParsers::from_string<std::string>, Config::ext_help_signals);

    /**
     * SIGALRM handler(s)
     */
    SST_CONFIG_DECLARE_OPTION(std::string, sigalrm, "",
        std::bind(&StandardConfigParsers::append_string, ";", "", std::placeholders::_1, std::placeholders::_2),
        Config::ext_help_signals);


    /**** Advanced options - Checkpointing ****/

     /**
     * Enable checkpointing for interactive debug
     */
    SST_CONFIG_DECLARE_OPTION(bool, checkpoint_enable, 0, &StandardConfigParsers::flag_set_true);

    /**
     * Interval at which to create a checkpoint in wall time
     */
    SST_CONFIG_DECLARE_OPTION(uint32_t, checkpoint_wall_period, 0, &StandardConfigParsers::wall_time_to_seconds);

    /**
     * Interval at which to create a checkpoint in simulated time
     */
    SST_CONFIG_DECLARE_OPTION(std::string, checkpoint_sim_period, "",
        std::bind(&StandardConfigParsers::check_unitalgebra_store_string, "s, Hz", std::placeholders::_1,
            std::placeholders::_2));

    /**
     * Returns whether the simulation will begin from a checkpoint (true) or not (false).
     */
    SST_CONFIG_DECLARE_OPTION(bool, load_from_checkpoint, false, &StandardConfigParsers::flag_set_true);

    /**
       Prefix for checkpoint filenames and directory
    */
    static std::string ext_help_checkpoint_prefix();

    SST_CONFIG_DECLARE_OPTION(std::string, checkpoint_prefix, "checkpoint", &StandardConfigParsers::nonempty_string,
        &Config::ext_help_checkpoint_prefix);

    /**
       Format for checkout filenames
    */
    static int parse_checkpoint_name_format(std::string& var, std::string arg);

    static std::string ext_help_checkpoint_format();

    SST_CONFIG_DECLARE_OPTION(std::string, checkpoint_name_format, "%p_%n_%t/%p_%n_%t",
        std::bind(&Config::parse_checkpoint_name_format, std::placeholders::_1, std::placeholders::_2),
        &Config::ext_help_checkpoint_format);

public:

    /** Get whether or not any of the checkpoint options were turned on */
    bool canInitiateCheckpoint();

    /** Print to stdout the current configuration */
    void print();

    void merge_checkpoint_options(Config& other);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Config);


protected:
    std::string getUsagePrelude() override;
    int         checkArgsAfterParsing() override;
    int         positionalCallback(int num, const std::string& arg);

private:
    //// Items private to Config

    std::string run_name;
    bool        first_rank_ = false;

    // Inserts all the command line options into the underlying data
    // structures
    void insertOptions();

    bool isFileNameOnly(const std::string& name)
    {
        bool nameOnly = true;

        for ( size_t i = 0; i < name.size(); ++i ) {
            if ( '/' == name[i] ) {
                nameOnly = false;
                break;
            }
        }

        return nameOnly;
    }
};

} // namespace SST

#endif // SST_CORE_CONFIG_H
