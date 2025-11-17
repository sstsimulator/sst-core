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

#ifndef SST_CORE_UTIL_BASIC_PERF_H
#define SST_CORE_UTIL_BASIC_PERF_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace SST {
class Output;

namespace Util {

struct RegionPerfInfo
{
    double      begin_time    = 0.0;
    double      end_time      = 0.0;
    uint64_t    begin_mem     = 0;
    uint64_t    end_mem       = 0;
    size_t      level         = 0;
    bool        last_of_level = true;
    bool        has_child     = false;
    std::string tag;

    // Roll-up data

    // First entry will be the max duration.  If detailed is turned on
    // (i.e. report all rank data), then all the rank data will come
    // after, with index 1 being rank 0, etc.
    std::vector<double> rollup_duration;

    // Rank with the max duration
    int rollup_max_duration_rank = -1;

    // This tracks the begin memory.  Index 0 will be the total
    // global memory, index 1 will be the max memory on any one
    // rank. If detailed is turned on (i.e. report all rank data),
    // then all the rank data will come after, with index 2 being rank
    // 0, etc.
    std::vector<uint64_t> rollup_begin_mem;

    // Rank with the max memory
    int rollup_begin_mem_max_rank = -1;

    // This tracks the end memory.  Index 0 will be the total
    // global memory, index 1 will be the max memory on any one
    // rank. If detailed is turned on (i.e. report all rank data),
    // then all the rank data will come after, with index 2 being rank
    // 0, etc.
    std::vector<uint64_t> rollup_end_mem;

    // Rank with the max memory
    int rollup_end_mem_max_rank = -1;

    RegionPerfInfo(const std::string& tag, size_t level) :
        level(level),
        tag(tag)
    {}
};

/**
   Class used to track various performance data during simulation
   execution.

   Regions:

   Regions are tracked hierarchically and you can output various
   levels of data based on the verbose value supplied to the output
   functions.  Each region is denoted using beginRegion() and
   endRegion().  Each region can be contained with another region, but
   it must be wholly contained within that region. For output, the
   verbose value can be used to indicate how much detail to output.
   The verbose value indicates how deep to print in the region
   hierarchy.  A verbose value of 0 indicates no output, 1 will output
   only the top level regions, etc.  The default verbose level is 2.

   Scalars:

   Scalars are tracked using the addMetric<>() function and are
   retrieved by using the getMetric<>() function.  These can only
   track signed and unsigned ints and floating point numbers.  All
   types are stored as their 64-bit versions.
 */
class BasicPerfTracker
{
public:

    void initialize(int rank, int num_ranks);

    RegionPerfInfo getRegionPerfInfo(const std::string& name);

    /**
       Begin a new code region
     */
    void beginRegion(const std::string& tag);

    /**
       End the named region.  This will cause a series of collectives
       to gather the total and max resource utilizations for the
       region.  This data will be kept on all ranks.
     */
    void endRegion(const std::string& tag);

    ///// Functions to get the local and global information about the execution /////

    /**
       Get the local begin time for the specified region
     */
    double getRegionBeginTime(const std::string& tag);

    /**
       Get the local ending time for the specified region
     */
    double getRegionEndTime(const std::string& tag);

    /**
       Get the local duration for the specified region
     */
    double getRegionDuration(const std::string& tag);

    /**
       Get the global duration for the specified region
     */
    double getRegionGlobalDuration(const std::string& tag);

    ///// Functions to get the local and global information about the memory usage /////

    /**
       Get the local memory size at begin for the specified region

       @return local memory usage in bytes
    */
    uint64_t getLocalRegionBeginMemSize(const std::string& tag);

    /**
       Get the global total memory size at begin for the specified region

       @return global total memory usage in bytes
    */
    uint64_t getGlobalTotalRegionBeginMemSize(const std::string& tag);

    /**
       Get the global max memory size at begin for the specified region

       @return pair of max memory usage in bytes and the rank that
       usage was from
    */
    std::pair<uint64_t, int> getGlobalMaxRegionBeginMemSize(const std::string& tag);


    /**
       Get the local memory size at end for the specified region

       @return local memory usage in bytes
    */
    uint64_t getLocalRegionEndMemSize(const std::string& tag);

    /**
       Get the global total memory size at end for the specified region

       @return global total memory usage in bytes
    */
    uint64_t getGlobalTotalRegionEndMemSize(const std::string& tag);

    /**
       Get the global max memory size at end for the specified region

       @return pair of max memory usage in bytes and the rank that
       usage was from
    */
    std::pair<uint64_t, int> getGlobalMaxRegionEndMemSize(const std::string& tag);


    void addMetric(const std::string& name, uint64_t value);
    void addMetric(const std::string& name, int64_t value);
    void addMetric(const std::string& name, double value);

    uint64_t getMetricUnsigned(const std::string& name);
    int64_t  getMetricSigned(const std::string& name);
    double   getMetricFloat(const std::string& name);

    void outputRegionData(Output& out, size_t verbose);

private:

    /**
       Stores the regions in the order they are created
     */
    std::vector<RegionPerfInfo> regions_;

    /**
       Stack of currently active regions
     */
    std::stack<size_t> current_regions_;

    /**
       Gets the specified region from the regions_ vector.  If it's
       not there, prints an error message that references the passed
       in function name and calls SST_Exit().
     */
    RegionPerfInfo& getRegion(const std::string& tag, const std::string& function_name, bool must_be_ended = true);


    // Scalar storage
    std::map<std::string, uint64_t> scalars_unsigned;
    std::map<std::string, int64_t>  scalars_signed;
    std::map<std::string, double>   scalars_float;

    int        rank_;
    int        num_ranks_;
    // TEMP
    static int level;
};


} // namespace Util
} // namespace SST

#endif // SST_CORE_UTIL_BASIC_PERF_H
