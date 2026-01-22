// -*- c++ -*-

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

#include "sst/core/model/restart/sstcptmodel.h"

#include "sst/core/simulation_impl.h"

#include <cstdint>
#include <fstream>
#include <string>

// #include "nlohmann/json.hpp"
// using json = nlohmann::json;

namespace SST::Core {

SSTCPTModelDefinition::SSTCPTModelDefinition(
    const std::string& script_file, int UNUSED(verbosity), Config* config, double UNUSED(start_time)) :
    SSTModelDescription(config),
    manifest_(script_file),
    config_(config)
{}

ConfigGraph*
SSTCPTModelDefinition::createConfigGraph()
{
    Config& cfg = Simulation_impl::config;

    SST::Core::Serialization::serializer ser;
    ser.enable_pointer_tracking();
    std::vector<char> restart_data_buffer;

    std::ifstream fs(cfg.configFile());
    if ( !fs.is_open() ) {
        if ( fs.bad() ) {
            fprintf(stderr, "Unable to open checkpoint file [%s]: badbit set\n", cfg.configFile().c_str());
            SST_Exit(-1);
        }
        if ( fs.fail() ) {
            fprintf(stderr, "Unable to open checkpoint file [%s]: %s\n", cfg.configFile().c_str(), strerror(errno));
            SST_Exit(-1);
        }
        fprintf(stderr, "Unable to open checkpoint file [%s]: unknown error\n", cfg.configFile().c_str());
        SST_Exit(-1);
    }

    std::string checkpoint_directory = cfg.configFile().substr(0, cfg.configFile().find_last_of('/'));

    std::string line;

    // Look for the line that has the global data file
    std::string globals_filename;
    std::string search_str("** (globals): ");
    while ( std::getline(fs, line) ) {
        // Look for lines starting with "** (globals):", then get the filename.
        size_t pos = line.find(search_str);
        if ( pos == 0 ) {
            // Get the file name
            globals_filename = checkpoint_directory + "/" + line.substr(search_str.length());
            break;
        }
    }
    fs.close();

    // Need to open the globals file
    std::ifstream fs_globals(globals_filename);
    if ( !fs_globals.is_open() ) {
        if ( fs_globals.bad() ) {
            fprintf(stderr, "XXUnable to open checkpoint globals file [%s]: badbit set\n", globals_filename.c_str());
            SST_Exit(-1);
        }
        if ( fs_globals.fail() ) {
            fprintf(stderr, "YYUnable to open checkpoint globals file [%s]: %s\n", globals_filename.c_str(),
                strerror(errno));
            SST_Exit(-1);
        }
        fprintf(stderr, "ZZUnable to open checkpoint globals file [%s]: unknown error\n", globals_filename.c_str());
        SST_Exit(-1);
    }

    size_t size;

    fs_globals.read(reinterpret_cast<char*>(&size), sizeof(size));
    restart_data_buffer.resize(size);
    fs_globals.read(restart_data_buffer.data(), size);

    Config       cpt_config;
    ConfigGraph* graph = new ConfigGraph();

    ser.start_unpacking(restart_data_buffer.data(), size);

    SST_SER(cpt_config);
    cfg.merge_checkpoint_options(cpt_config);

    SST_SER(graph->cpt_ranks.rank);
    SST_SER(graph->cpt_ranks.thread);
    SST_SER(graph->cpt_currentSimCycle);
    SST_SER(graph->cpt_currentPriority);

    // Deserialization continues below

    ////// Check to make sure we have the right parallelism in the restart

    /******** vv Works for regular and N->1 restart vv ***********/
    // Check to make sure that the checkpoint and restart
    // parallelism match or if we are restarting with a serial
    // run.  The N->1 restart is a special case and a step towards
    // general repartitioned restarts.
    if ( (cfg.num_ranks() != graph->cpt_ranks.rank || cfg.num_threads() != graph->cpt_ranks.thread) &&
         !(cfg.num_threads() == 1 && cfg.num_ranks() == 1) ) {

        Output::getDefaultObject().fatal(CALL_INFO, 1,
            "Rank or thread counts do not match checkpoint. "
            "Checkpoint requires %" PRIu32 " ranks and %" PRIu32 " threads. "
            "Serial restarts are also permitted.\n",
            graph->cpt_ranks.rank, graph->cpt_ranks.thread);
    }
    /******** ^^ Works for regular and N->1 restart ^^ ***********/

    ////// Initialize global data //////


    // // On restart runs, need to reinitialize the loaded libraries
    // // and SharedObject::manager
    // Factory* factory = Factory::createFactory(cfg.getLibPath());

    // Get set of loaded libraries
    SST_SER(*(graph->cpt_libnames.get()));

    // Get the checkpoint system info
    SST_SER(version_);
    SST_SER(arch_);
    SST_SER(os_);

    ////// Check to make sure we have the right SSTCore version, architecture and operating system
    if ( version_ != PACKAGE_STRING ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1,
            "Version mismatch in SST checkpoint file.  SSTCore version is %s. "
            "Checkpoint version is %s\n",
            PACKAGE_VERSION, version_.c_str());
    }

    std::string tmp_arch = "";
#if defined(__x86_64__) || defined(_M_X64)
    tmp_arch = "x86_64";
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    tmp_arch = "x86_32";
#elif defined(__ARM_ARCH_2__)
    tmp_arch = "ARM2";
#elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
    tmp_arch = "ARM3";
#elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
    tmp_arch = "ARM4T";
#elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
    tmp_arch = "ARM5"
#elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
    tmp_arch = "ARM6T2";
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || \
    defined(__ARM_ARCH_6ZK__)
    tmp_arch = "ARM6";
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || \
    defined(__ARM_ARCH_7S__)
    tmp_arch = "ARM7";
#elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    tmp_arch = "ARM7A";
#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    tmp_arch = "ARM7R";
#elif defined(__ARM_ARCH_7M__)
    tmp_arch = "ARM7M";
#elif defined(__ARM_ARCH_7S__)
    tmp_arch = "ARM7S";
#elif defined(__aarch64__) || defined(_M_ARM64)
    tmp_arch = "ARM64";
#elif defined(mips) || defined(__mips__) || defined(__mips)
    tmp_arch = "MIPS";
#elif defined(__sh__)
    tmp_arch = "SUPERH";
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
    defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
    tmp_arch = "POWERPC";
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
    tmp_arch = "POWERPC64";
#elif defined(__sparc__) || defined(__sparc)
    tmp_arch = "SPARC";
#elif defined(__m68k__)
    tmp_arch = "M68K";
#elif defined(__riscv__) || defined(_riscv) || defined(__riscv)
    tmp_arch = "RISCV";
#else
    tmp_arch = "UNKNOWN";
#endif

    if ( arch_ != tmp_arch ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1,
            "Architecture mismatch in SST checkpoint file.  Current architecture is %s. "
            "Checkpointed architecture is %s\n",
            tmp_arch.c_str(), arch_.c_str());
    }

    std::string tmp_os = "";
#if defined(_WIN32) || defined(_WIN64)
    tmp_os = "OS_WINDOWS";
#elif defined(__APPLE__) && defined(__MACH__)
    tmp_os = "OS_MACOS";
#elif defined(__linux__)
    tmp_os = "OS_LINUX";
#elif defined(__unix__) || defined(__unix)
    tmp_os = "OS_UNIX";
#elif defined(__FreeBSD__)
    tmp_os = "OS_FREEBSD";
#else
    tmp_os = "OS_UNKNOWN";
#endif

    if ( os_ != tmp_os ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1,
            "Operating system mismatch in SST checkpoint file.  Current OS is %s. "
            "Checkpointed OS is %s\n",
            tmp_os.c_str(), os_.c_str());
    }

    // factory->loadUnloadedLibraries(libnames);

    // Get shared object blob
    fs_globals.read(reinterpret_cast<char*>(&size), sizeof(size));
    graph->cpt_shared_objects.get()->resize(size);
    fs_globals.read(graph->cpt_shared_objects.get()->data(), size);

    // Get stats config blob
    fs_globals.read(reinterpret_cast<char*>(&size), sizeof(size));
    graph->cpt_stats_config.get()->resize(size);
    fs_globals.read(graph->cpt_stats_config.get()->data(), size);


    fs_globals.read(reinterpret_cast<char*>(&size), sizeof(size));
    restart_data_buffer.resize(size);
    ser.start_unpacking(restart_data_buffer.data(), size);

    // Duplicate data that we can remove
    RankInfo rank;
    SST_SER(rank);

    SST_SER(graph->cpt_minPart);
    SST_SER(graph->cpt_minPartTC);
    SST_SER(graph->cpt_max_event_id);


    // // Initialize SharedObjectManager
    // Simulation_impl::serializeSharedObjectManager(ser);
    // // SST_SER(SST::Shared::SharedObject::manager);

    // // Get the stats config
    // SST_SER(Simulation_impl::stats_config_);

    // Done with restart_data_buffer
    restart_data_buffer.clear();

    fs_globals.close();
    setOptionFromModel("load-checkpoint", "");
    return graph;
}


} // namespace SST::Core
