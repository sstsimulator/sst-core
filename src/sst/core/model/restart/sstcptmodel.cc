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

SSTCPTModelDefinition::~SSTCPTModelDefinition() {}


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

    std::string checkpoint_directory = cfg.configFile().substr(0, cfg.configFile().find_last_of("/"));

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
