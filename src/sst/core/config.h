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

class ConfigHelper;
class SSTModelDescription;
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

private:
    // Main creates the config object
    friend int ::main(int argc, char** argv);
    friend class ConfigHelper;
    friend class SSTModelDescription;

    /**
       Config constructor.  Meant to only be created by main function
     */
    Config(uint32_t num_ranks, bool first_rank);

    /**
       Default constructor used for serialization.  At this point,
       first_rank_ is no longer needed, so just initialize to false.
     */
    Config() :
        ConfigShared(true, {}),
        first_rank_(false)
    {}

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


    /**
       Number of threads requested
    */
    uint32_t num_threads() const { return num_threads_; }

    /**
       Number of ranks in the simulation
     */
    uint32_t num_ranks() const { return num_ranks_; }

    /**
       Name of the SDL file to use to genearte the simulation
    */
    const std::string& configFile() const { return configFile_; }

    /**
       Model options to pass to the SDL file
    */
    const std::string& model_options() const { return model_options_; }

    /**
       Print SST timing information after the run
    */
    bool print_timing() const { return print_timing_; }

    /**
       File to output json formatted config graph to (empty string means no
       output)
    */
    const std::string& timing_json() const { return timing_json_; }

    /**
       Simulated cycle to stop the simulation at
    */
    const std::string& stop_at() const { return stop_at_; }

    /**
       Wall clock time (approximiate) in seconds to stop the simulation at
    */
    uint32_t exit_after() const { return exit_after_; }

    /**
       Partitioner to use for parallel simulations
    */
    const std::string& partitioner() const { return partitioner_; }

    /**
       Simulation period at which to print out a "heartbeat" message
    */
    const std::string& heartbeat_sim_period() const { return heartbeat_sim_period_; }

    /**
       Wall-clock period at which to print out a "heartbeat" message
    */
    uint32_t heartbeat_wall_period() const { return heartbeat_wall_period_; }

    /**
       The directory to be used for writing output files
    */
    const std::string& output_directory() const { return output_directory_; }

    /**
       Prefix to use for the default SST::Output object in core
    */
    const std::string output_core_prefix() const { return output_core_prefix_; }

    // Configuration output

    /**
       File to output python formatted  config graph to (empty string means no
       output)
    */
    const std::string& output_config_graph() const { return output_config_graph_; }

    /**
       File to output json formatted config graph to (empty string means no
       output)
    */
    const std::string& output_json() const { return output_json_; }

    /**
       If true, and a config graph output option is specified, write
       each ranks graph separately
    */
    bool parallel_output() const { return parallel_output_; }


    // Graph output

    /**
       File to output dot formatted config graph to (empty string means no
       output).  Note, this is not a format that can be used as input for simulation

    */
    const std::string& output_dot() const { return output_dot_; }

    /**
       Level of verbosity to use for the dot output.
    */
    uint32_t dot_verbosity() const { return dot_verbosity_; }

    /**
       File to output component partition info to (empty string means no output)
    */
    const std::string& component_partition_file() const { return component_partition_file_; }

    /**
       Controls whether partition info is output as part of configuration output
     */
    bool output_partition() const { return output_partition_; }

    // Advanced options

    /**
       Core timebase to use as the atomic time unit for the
       simulation.  It is usually best to just leave this at the
       default (1ps)
    */
    const std::string& timeBase() const { return timeBase_; }

    /**
       Controls whether graph constuction will be done in parallel.
       If it is, then the SDL file name is modified to add the rank
       number to the file name right before the file extension, if
       parallel_load_mode_multi is true.
    */
    bool parallel_load() const { return parallel_load_; }

    /**
       If graph constuction will be done in parallel, will use a
       file per rank if true, and the same file for each rank if
       false.
    */
    bool parallel_load_mode_multi() const { return parallel_load_mode_multi_; }

    /**
       Returns the string equivalent for parallel-load: NONE (if
       parallel load is off), SINGLE or MULTI.
    */
    std::string parallel_load_str() const
    {
        if ( !parallel_load_ ) return "NONE";
        if ( parallel_load_mode_multi_ ) return "MULTI";
        return "SINGLE";
    }

    /**
     * Interval at which to create a checkpoint in wall time
     */
    uint32_t           checkpoint_wall_period() const { return checkpoint_wall_period_; }
    /**
     * Interval at which to create a checkpoint in simulated time
     */
    const std::string& checkpoint_sim_period() const { return checkpoint_sim_period_; }

    /**
     * Returns whether the simulation will begin from a checkpoint (true) or not (false).
     */
    bool load_from_checkpoint() const { return load_from_checkpoint_; }

    /**
       Prefix for checkpoint filenames and directory
    */
    const std::string& checkpoint_prefix() const { return checkpoint_prefix_; }

    /**
       Format for checkout filenames
    */
    const std::string& checkpoint_name_format() const { return checkpoint_name_format_; }

    /**
       TimeVortex implementation to use
    */
    const std::string& timeVortex() const { return timeVortex_; }

    /**
       Use links that connect directly to ActivityQueue in receiving thread
    */
    bool interthread_links() const { return interthread_links_; }

#ifdef USE_MEMPOOL
    /**
       Controls whether mempool items are cache-aligned

    */
    bool cache_align_mempools() const { return cache_align_mempools_; }
#endif
    /**
       File to which core debug information should be written
    */
    const std::string& debugFile() const { return debugFile_; }

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
    bool enablePythonCoverage() const { return enable_python_coverage_; }
#endif

    // Advanced options - Profiling

    /**
       Profiling points to turn on
     */
    const std::string& enabledProfiling() const { return enabled_profiling_; }

    /**
       Profiling points to turn on
     */
    const std::string& profilingOutput() const { return profiling_output_; }

    // Advanced options - Debug

    /**
       Run mode to use (Init, Both, Run-only).  Note that Run-only is
       not currently supported because there is not component level
       checkpointing.
    */
    SimulationRunMode runMode() const { return runMode_; }

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

    /**
       Get the InteractiveAction to use for interactive mode
     */
    std::string interactive_console() const { return interactive_console_; }

    /**
       Get the time to start interactive mode
    */
    std::string interactive_start_time() const { return interactive_start_time_; }


#ifdef USE_MEMPOOL
    /**
       File to output list of events that remain undeleted at the end
       of the simulation.

       If no mempools, just reutrn empty string.  This avoids a check
    for mempools in main.cc
    */
    const std::string& event_dump_file() const { return event_dump_file_; }
#endif

    /**
       Run simulation initialization stages one rank at a time for
       debug purposes
     */
    bool rank_seq_startup() const { return rank_seq_startup_; }

    // Advanced options - envrionment

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
    bool enable_sig_handling() const { return enable_sig_handling_; }

    /**
     * SIGUSR1 handler
     */
    const std::string& sigusr1() const { return sigusr1_; }

    /**
     * SIGUSR2 handler
     */
    const std::string& sigusr2() const { return sigusr2_; }

    /**
     * SIGALRM handler(s)
     */
    const std::string& sigalrm() const { return sigalrm_; }

    // This option is used by the SST wrapper found in
    // bootshare.{h,cc} and is never actually accessed once sst.x
    // executes.


    /** Get whether or not any of the checkpoint options were turned on */
    bool canInitiateCheckpoint();

    /** Print to stdout the current configuration */
    void print();

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(verbose_);
        SST_SER(configFile_);
        SST_SER(model_options_);
        SST_SER(print_timing_);
        SST_SER(timing_json_);
        SST_SER(stop_at_);
        SST_SER(exit_after_);
        SST_SER(partitioner_);
        SST_SER(heartbeat_wall_period_);
        SST_SER(heartbeat_sim_period_);
        SST_SER(output_directory_);
        SST_SER(output_core_prefix_);

        SST_SER(output_config_graph_);
        SST_SER(output_json_);
        SST_SER(parallel_output_);

        SST_SER(output_dot_);
        SST_SER(dot_verbosity_);
        SST_SER(component_partition_file_);
        SST_SER(output_partition_);

        SST_SER(timeBase_);
        SST_SER(parallel_load_);
        SST_SER(parallel_load_mode_multi_);
        SST_SER(timeVortex_);
        SST_SER(interthread_links_);
#ifdef USE_MEMPOOL
        SST_SER(cache_align_mempools_);
#endif
        SST_SER(debugFile_);
        SST_SER(libpath_);
        SST_SER(addlibpath_);
#if PY_MINOR_VERSION >= 9
        SST_SER(enable_python_coverage_);
#endif
        SST_SER(enabled_profiling_);
        SST_SER(profiling_output_);
        SST_SER(runMode_);
        SST_SER(interactive_console_);
        SST_SER(interactive_start_time_);
#ifdef USE_MEMPOOL
        SST_SER(event_dump_file_);
#endif
        SST_SER(load_from_checkpoint_);
        SST_SER(checkpoint_wall_period_);
        SST_SER(checkpoint_sim_period_);
        SST_SER(checkpoint_prefix_);
        SST_SER(checkpoint_name_format_);

        SST_SER(enable_sig_handling_);
        SST_SER(sigusr1_);
        SST_SER(sigusr2_);
        SST_SER(sigalrm_);
        SST_SER(print_env_);
        SST_SER(no_env_config_);
    }
    ImplementSerializable(SST::Config);

protected:
    std::string getUsagePrelude() override;
    int         checkArgsAfterParsing() override;
    int         positionalCallback(int num, const std::string& arg);

private:
    //// Items private to Config

    std::string run_name;
    bool        first_rank_;

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

    // Variables to hold the options.  Declared in order they show up
    // in the options array

    // Information options
    // No variable associated with help
    // No variable associated with version

    // Basic options
    // uint32_t    verbose_; ** in ConfigShared
    // Num threads held in RankInfo.thread
    uint32_t    num_ranks_;             /*!< Number of ranks in the simulation */
    uint32_t    num_threads_;           /*!< Number of threads requested */
    std::string configFile_;            /*!< Graph generation file */
    std::string model_options_;         /*!< Options to pass to Python Model generator */
    bool        print_timing_;          /*!< Print SST timing information */
    std::string timing_json_;           /*!< File to save JSON formated SST timing information */
    std::string stop_at_;               /*!< When to stop the simulation */
    uint32_t    exit_after_;            /*!< When (wall-time) to stop the simulation */
    std::string partitioner_;           /*!< Partitioner to use */
    std::string heartbeat_sim_period_;  /*!< Sets the heartbeat (simulated time) period for the simulation */
    uint32_t    heartbeat_wall_period_; /*!< Sets the heartbeat (wall-clock time) period for the simulation */
    std::string output_directory_;      /*!< Output directory to dump all files to */
    std::string output_core_prefix_;    /*!< Set the SST::Output prefix for the core */

    // Configuration output
    std::string output_config_graph_; /*!< File to dump configuration graph */
    std::string output_json_;         /*!< File to dump JSON output */
    bool        parallel_output_;     /*!< Output simulation graph in parallel */

    // Graph output
    std::string output_dot_;               /*!< File to dump dot output */
    uint32_t    dot_verbosity_;            /*!< Amount of detail to include in the dot graph output */
    std::string component_partition_file_; /*!< File to dump component graph */
    bool        output_partition_;         /*!< Output paritition info when writing config output */

    // Advanced options
    std::string timeBase_;                 /*!< Timebase of simulation */
    bool        parallel_load_;            /*!< Load simulation graph in parallel */
    bool        parallel_load_mode_multi_; /*!< If true, load using multiple files */
    std::string timeVortex_;               /*!< TimeVortex implementation to use */
    bool        interthread_links_;        /*!< Use interthread links */
#ifdef USE_MEMPOOL
    bool cache_align_mempools_; /*!< Cache align allocations from mempools */
#endif
    std::string debugFile_; /*!< File to which debug information should be written */
    // std::string libpath_;  ** in ConfigShared
    // std::string addLibPath_; ** in ConfigShared
#if PY_MINOR_VERSION >= 9
    bool enable_python_coverage_; /*!< Enable the Python coverage module */
#endif

    // Advanced options - profiling
    std::string enabled_profiling_; /*!< Enabled default profiling points */
    std::string profiling_output_;  /*!< Location to write profiling data */

    // Advanced options - debug
    SimulationRunMode runMode_;                /*!< Run Mode (Init, Both, Run-only) */
    std::string       interactive_console_;    /*!< Action to use for interactive mode */
    std::string       interactive_start_time_; /*!< Time to drop into interactive mode */
#ifdef USE_MEMPOOL
    std::string event_dump_file_; /*!< File to dump undeleted events to  */
#endif
    bool rank_seq_startup_; /*!< Run simulation initialization phases one rank at a time */

    // Advanced options - checkpoint
    bool        load_from_checkpoint_;  /*!< If true, load from checkpoint instead of config file */
    std::string checkpoint_sim_period_; /*!< Interval to generate checkpoints at in terms of the simulated clock */
    uint32_t
        checkpoint_wall_period_;    /*!< Interval to generate checkpoints at in terms of wall-clock measured seconds */
    std::string checkpoint_prefix_; /*!< Prefix for checkpoint filename and checkpoint directory */
    std::string checkpoint_name_format_; /*!< Format for checkpoint filenames */

    // Advanced options - environment
    bool        enable_sig_handling_; /*!< Enable signal handling */
    std::string sigusr1_;             /*!< RealTimeAction to call on a SIGUSR1 */
    std::string sigusr2_;             /*!< RealTimeAction to call on a SIGUSR2 */
    std::string sigalrm_;             /*!< RealTimeAction(s) to call on a SIGALRM */
    // bool print_env_;  ** in ConfigShared
    // bool no_env_config_; ** in ConfigShared
};

} // namespace SST

#endif // SST_CORE_CONFIG_H
