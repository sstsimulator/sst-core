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

#ifndef SST_CORE_TIMING_OUTPUT_H
#define SST_CORE_TIMING_OUTPUT_H

#include "sst/core/output.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/util/filesystem.h"

#include <map>

namespace SST::Core {

/**
 * Outputs configuration data to a specified file path.
 */
class TimingOutput
{
public:
    /** Timing Parameters
     */
    enum Key {
        LOCAL_MAX_RSS,             // Max Resident Set Size (kB)
        GLOBAL_MAX_RSS,            // Approx. Global Max RSS Size (kB)
        LOCAL_MAX_PF,              // Max Local Page Faults
        GLOBAL_PF,                 // Global Page Faults
        GLOBAL_MAX_IO_IN,          // Max Output Blocks
        GLOBAL_MAX_IO_OUT,         // Max Input Blocks
        GLOBAL_MAX_SYNC_DATA_SIZE, // Max Sync data size
        GLOBAL_SYNC_DATA_SIZE,     // Global Sync data size
        MAX_MEMPOOL_SIZE,          // Max mempool usage (bytes)
        GLOBAL_MEMPOOL_SIZE,       // Global mempool usage (bytes)
        MAX_BUILD_TIME,            // Build time (wallclock seconds)
        MAX_RUN_TIME,              // Run loop time (wallclock seconds)
        MAX_TOTAL_TIME,            // Total time (wallclock seconds)
        SIMULATED_TIME_UA,         // Simulated time ( Algebra seconds string. eg. "10 us" )
        GLOBAL_ACTIVE_ACTIVITIES,  // Global active activities
        GLOBAL_CURRENT_TV_DEPTH,   // Current global TimeVortex depth
        GLOBAL_MAX_TV_DEPTH,       // Max TimeVortex depth
        RANKS,                     // MPI ranks
        THREADS,                   // Threads
    };

    const std::map<Key, const char*> key2cstr = {
        { LOCAL_MAX_RSS, "local_max_rss" },
        { GLOBAL_MAX_RSS, "global_max_rss" },
        { LOCAL_MAX_PF, "local_max_pf" },
        { GLOBAL_PF, "global_pf" },
        { GLOBAL_MAX_IO_IN, "global_max_io_in" },
        { GLOBAL_MAX_IO_OUT, "global_max_io_out" },
        { GLOBAL_MAX_SYNC_DATA_SIZE, "global_max_sync_data_size" },
        { GLOBAL_SYNC_DATA_SIZE, "global_sync_data_size" },
        { MAX_MEMPOOL_SIZE, "max_mempool_size" },
        { GLOBAL_MEMPOOL_SIZE, "global_mempool_size" },
        { MAX_BUILD_TIME, "max_build_time" },
        { MAX_RUN_TIME, "max_run_time" },
        { MAX_TOTAL_TIME, "max_total_time" },
        { SIMULATED_TIME_UA, "simulated_time_ua" },
        { GLOBAL_ACTIVE_ACTIVITIES, "global_active_activities" },
        { GLOBAL_CURRENT_TV_DEPTH, "global_current_tv_depth" },
        { GLOBAL_MAX_TV_DEPTH, "global_max_tv_depth" },
        { RANKS, "ranks" },
        { THREADS, "threads" },
    };

    TimingOutput(const SST::Output& output, bool printEnable);
    virtual ~TimingOutput();
    void setJSON(const std::string& path);
    void generate();
    void renderText();
    void renderJSON();

    void set(Key key, uint64_t v);
    void set(Key key, UnitAlgebra v);
    void set(Key key, double v);

private:
    SST::Output output_;
    bool        printEnable_;
    bool        jsonEnable_;

    std::map<Key, uint64_t>    u64map_    = {};
    std::map<Key, UnitAlgebra> uamap_     = {};
    std::map<Key, double>      dmap_      = {};
    FILE*                      outputFile = nullptr;
};

} // namespace SST::Core

#endif // SST_CORE_TIMING_OUTPUT_H
