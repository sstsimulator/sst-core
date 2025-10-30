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

#include "sst/core/checkpointAction.h"

#include "sst/core/component.h"
#include "sst/core/mempoolAccessor.h"
#include "sst/core/objectComms.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"

#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace SST {

namespace pvt {

std::string
createNameFromFormat(const std::string& format, const std::string& prefix, uint64_t checkpoint_id, SimTime_t time)
{
    std::string ret;
    bool        found_percent = false;
    for ( const auto& x : format ) {
        if ( found_percent ) {
            switch ( x ) {
            case 'p':
                ret += prefix;
                break;
            case 'n':
                ret += std::to_string(checkpoint_id);
                break;
            case 't':
                ret += std::to_string(time);
                break;
            default:
                // Should not happen since format string was already
                // checked, but if it does, just delete whole %
                // sequence (i.e. do nothing)
                break;
            }
            found_percent = false;
        }
        else if ( x == '%' ) {
            found_percent = true;
        }
        else {
            ret += x;
        }
    }
    return ret;
}

} // namespace pvt

CheckpointAction::CheckpointAction(Config* cfg, RankInfo this_rank, Simulation_impl* sim, TimeConverter* period) :
    Action(),
    rank_(this_rank),
    period_(period),
    generate_(false)
{
    // Set the priority to be the same as the SyncManager so that
    // checkpointing happens in the same place for both serial and
    // parallel runs.  We will never have both a SyncManager and a
    // CheckpointAction in the TimeVortex at the same time since the
    // SyncManager manages the CheckpointAction on parallel runs.
    setPriority(SYNCPRIORITY);

    next_sim_time_ = 0;
    last_cpu_time_ = 0;

    // If period_ is set, then we have a simtime checkpoint period
    if ( period_ ) {
        // Compute the first checkpoint time
        next_sim_time_ =
            (period_->getFactor() * (sim->getCurrentSimCycle() / period_->getFactor())) + period_->getFactor();
    }
    else {
        next_sim_time_ = MAX_SIMTIME_T;
    }

    // Parse the format string.  It was checked by the Config object
    // to make sure there was no more than one directory separator (/)
    // and that no invalid % sequences were used.
    std::string format = cfg->checkpoint_name_format();
    size_t      split  = format.find("/");
    if ( split == format.npos ) {
        dir_format_  = format;
        file_format_ = format;
    }
    else {
        dir_format_  = format.substr(0, split);
        file_format_ = format.substr(split + 1);
    }

    if ( (0 == this_rank.rank) ) {
        // Check to make sure that there is at least one of %n or %t to
        // make checkpoint filenames unique.
        bool unique = false;
        if ( format.find("%n") != format.npos ) unique = true;
        if ( format.find("%t") != format.npos ) unique = true;

        if ( !unique ) {
            sim->getSimulationOutput().output(
                "WARNING: checkpoint-name-format does not include one of %%n or %%t, which means that all checkpoints "
                "will use the same filename and previous files will be overwritten [%s].\n",
                format.c_str());
        }

        last_cpu_time_ = sst_get_cpu_time();
    }
}

void
CheckpointAction::insertIntoTimeVortex(Simulation_impl* sim)
{
    // Only need to insert if a period was set
    if ( MAX_SIMTIME_T == next_sim_time_ ) return;

    // If this is a serial job, insert this into
    // the time TimeVortex.  If it is parallel, then the
    // CheckpointAction is managed by the SyncManager.
    RankInfo num_ranks = sim->getNumRanks();
    if ( num_ranks.rank == 1 && num_ranks.thread == 1 ) {
        sim->insertActivity(next_sim_time_, this);
    }
}


// Generate checkpoint on simulation time period
void
CheckpointAction::execute()
{
    Simulation_impl* sim = Simulation_impl::getSimulation();
    createCheckpoint(sim);

    next_sim_time_ += period_->getFactor();
    sim->insertActivity(next_sim_time_, this);
}

void
CheckpointAction::createCheckpoint(Simulation_impl* sim)
{
    if ( 0 == rank_.rank && 0 == rank_.thread ) {
        const double now = sst_get_cpu_time();
        sim->getSimulationOutput().output(
            "# Simulation Checkpoint: Simulated Time %s (Real CPU time since last checkpoint %.5f seconds)\n",
            sim->getElapsedSimTime().toStringBestSI().c_str(), (now - last_cpu_time_));

        last_cpu_time_ = now;
    }

    // Need to create a directory for this checkpoint
    std::string prefix = sim->checkpoint_prefix_;

    // Get the basename for the checkpoint directory using the checkpoint name format
    std::string basename = pvt::createNameFromFormat(dir_format_, prefix, checkpoint_id, sim->currentSimCycle);

    // Get the final checkpoint directory by adding the base checkpoint directory
    std::string directory = sim->checkpoint_directory_ + "/" + basename;

    // Only thread 0 will participate in setup
    if ( rank_.thread == 0 ) {
        // Rank 0 will create the directory for this checkpoint
        if ( rank_.rank == 0 ) {
            directory = sim->checkpoint_directory_ + "/" + basename;
            std::filesystem::create_directory(directory);
        }
#ifdef SST_CONFIG_HAVE_MPI
        Comms::broadcast(directory, 0);
#endif
    }

    // Get the basename for the files
    basename = pvt::createNameFromFormat(file_format_, prefix, checkpoint_id, sim->currentSimCycle);

    // Add the rank and thread info to the name
    std::string filename = basename + "_" + std::to_string(rank_.rank) + "_" + std::to_string(rank_.thread) + ".bin";

    barrier.wait();

    if ( rank_.thread == 0 ) checkpoint_id++;

    // Write out the checkpoints for the partitions
    sim->checkpoint(directory + "/" + filename);

    // Write out the registry.  Rank 0 thread 0 will write the global
    // state and its registry, then each thread will take a turn
    // writing its part of the registry
    RankInfo num_ranks = sim->getNumRanks();

    std::string registry_name = basename + ".sstcpt";

    // Need to write out the globals
    // Only rank0/thread0 writes, but all may need to participate in global data gather
    std::string globals_name = basename + "_globals.bin";
    sim->checkpoint_write_globals(checkpoint_id - 1, directory, registry_name, globals_name);

    // No need to barrier here since rank 0 thread 0 will be the first
    // to execute in the loop below and everything else will wait
    for ( uint32_t r = 0; r < num_ranks.rank; ++r ) {
        if ( r == rank_.rank ) {
            // If this is my rank go ahead
            for ( uint32_t t = 0; t < num_ranks.thread; ++t ) {
                // If this is my thread go ahead
                if ( t == rank_.thread ) {
                    sim->checkpoint_append_registry(directory + "/" + registry_name, filename);
                    barrier.wait();
                }
                else {
                    barrier.wait();
                }
            }
#ifdef SST_CONFIG_HAVE_MPI
            if ( rank_.thread == 0 ) {
                MPI_Barrier(MPI_COMM_WORLD);
                barrier.wait();
            }
            else {
                barrier.wait();
            }
#endif
        }
        else {
#ifdef SST_CONFIG_HAVE_MPI
            // If this isn't my rank, just barrier and wait until my
            // turn
            if ( rank_.thread == 0 ) {
                MPI_Barrier(MPI_COMM_WORLD);
                barrier.wait();
            }
            else {
                barrier.wait();
            }
#endif
        }
    }
}

// SyncManager check whether a checkpoint needs to be generated
SimTime_t
CheckpointAction::check(SimTime_t current_time)
{
    // The if-logic is a little weird, but it's trying to minimize the
    // number of branches in the normal case of no checkpoint being
    // initiated.  This will also handle the case where both a sim and
    // real-time trigger happened at the same time
    if ( (current_time == next_sim_time_) || generate_ ) {
        Simulation_impl* sim = Simulation_impl::getSimulation();
        createCheckpoint(sim);
        generate_ = false;
        // Only add to the simulation-interval checkpoint time if it
        // was what triggered this
        if ( current_time == next_sim_time_ ) {
            next_sim_time_ += period_->getFactor();
        }
    }
    return next_sim_time_;
}

void
CheckpointAction::setCheckpoint()
{
    generate_ = true;
}

SimTime_t
CheckpointAction::getNextCheckpointSimTime()
{
    return next_sim_time_;
}

Core::ThreadSafe::Barrier CheckpointAction::barrier;
uint32_t                  CheckpointAction::checkpoint_id = 1;

namespace Checkpointing {

/**
   Function to see if a directory exists.  We need this bacause
   std::filesystem isn't fully supported until GCC9.

   @param dirName name of directory to check

   @param include_files will also return true if a filename matches
   the input name

   @return true if dirName is a directory in the filesystem. If
   include_files is set to true, this will also return true if a file
   name matches the passed in name (i.e. the name is available for
   use)

*/
bool
doesDirectoryExist(const std::string& dirName, bool include_files)
{
    struct stat info;
    if ( stat(dirName.c_str(), &info) != 0 ) {
        // Directory does not exist
        return false;
    }
    else if ( info.st_mode & S_IFDIR ) {
        // Directory exists
        return true;
    }
    else {
        // Exists but it's a file, so return include_files
        return include_files;
    }
}


std::string
initializeCheckpointInfrastructure(Config* cfg, bool rt_can_ckpt, int myRank)
{
    ////// Check to see if checkpointing is enabled //////
    if ( !cfg->canInitiateCheckpoint() && !rt_can_ckpt ) return "";

    std::string checkpoint_dir_name = "";

    if ( myRank == 0 ) {
        SST::Util::Filesystem& fs = Simulation_impl::getSimulation()->filesystem;
        checkpoint_dir_name       = fs.createUniqueDirectory(cfg->checkpoint_prefix());
    }

#ifdef SST_CONFIG_HAVE_MPI
    // Broadcast the directory name
    Comms::broadcast(checkpoint_dir_name, 0);
#endif

    return checkpoint_dir_name;
}


} // namespace Checkpointing

} // namespace SST
