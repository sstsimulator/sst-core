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

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>


namespace SST {
namespace Util {
class PerfReporter;
}

namespace Core {


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

    TimingOutput(const SST::Output& output, int print_verbosity);
    virtual ~TimingOutput();
    void generate(SST::Util::PerfReporter* reporter);

    void set(Key key, uint64_t v);
    void set(Key key, UnitAlgebra v);
    void set(Key key, double v);

private:
    SST::Output output_;
    int         print_verbosity_;

    std::map<Key, uint64_t>    u64map_ = {};
    std::map<Key, UnitAlgebra> uamap_  = {};
    std::map<Key, double>      dmap_   = {};
};
} // namespace Core
} // namespace SST

#endif // SST_CORE_TIMING_OUTPUT_H
