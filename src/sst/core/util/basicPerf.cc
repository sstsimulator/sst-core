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

#include "sst/core/util/basicPerf.h"

#include "sst/core/cputimer.h"
#include "sst/core/memuse.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"

#include <clocale>
#include <cstdio>

namespace SST::Util {


void
BasicPerfTracker::initialize(int rank, int num_ranks)
{
    rank_      = rank;
    num_ranks_ = num_ranks;
}


RegionPerfInfo&
BasicPerfTracker::getRegion(const std::string& tag, const std::string& function_name, bool must_be_ended)
{
    for ( auto& x : regions_ ) {
        if ( tag == x.tag ) {
            if ( x.end_mem == 0 && must_be_ended ) {
                // ERROR, region has not ended
                fprintf(stderr, "Called BasicPerfTracker::%s() on region that has not ended: %s\n",
                    function_name.c_str(), tag.c_str());
                SST_Exit(1);
            }
            return x;
        }
    }

    // ERROR, region not started
    fprintf(stderr, "Called BasicPerfTracker::%s() on region that has not started: %s\n", function_name.c_str(),
        tag.c_str());
    SST_Exit(1);
}


RegionPerfInfo
BasicPerfTracker::getRegionPerfInfo(const std::string& tag)
{
    return getRegion(tag, "getRegionPerfInfo");
}

void
BasicPerfTracker::beginRegion(const std::string& tag)
{
    for ( auto& x : regions_ ) {
        if ( tag == x.tag ) {
            // ERROR, region already started
            fprintf(stderr, "Called BasicPerfTracker::beginRegion() on region that has already been started: %s\n",
                tag.c_str());
            SST_Exit(1);
        }
    }

    // Let my parent know that they have a child
    if ( current_regions_.size() != 0 ) {
        regions_[current_regions_.top()].has_child = true;
    }

    // Push the index that this region will be in the vector to the
    // stack
    current_regions_.push(regions_.size());

    size_t level = current_regions_.size();

    // Need to see if there are other regions in this level that need
    // to have last_of_level set to false.  Easier to do this before
    // we insert the current region
    for ( auto iter = regions_.rbegin(); iter != regions_.rend(); ++iter ) {
        // Find last item with current level.  If we ever hit a level
        // less than ours, then we are the first child at the level
        // and we can stop looking.
        if ( iter->level < level ) break;
        if ( iter->level == level ) {
            iter->last_of_level = false;
            break;
        }
    }

    // Create the region in the vector. The level will be the
    // current size of the current_regions_ stack.
    regions_.emplace_back(tag, current_regions_.size());
    RegionPerfInfo& region = regions_.back();

    region.begin_mem  = Core::localMemSize();
    region.begin_time = sst_get_cpu_time();
}

void
BasicPerfTracker::endRegion(const std::string& tag)
{
    // Current region must be ended before any enclosing regions can be ended
    RegionPerfInfo& region = regions_[current_regions_.top()];
    if ( tag != region.tag ) {
        fprintf(stderr,
            "Called BasicPerfTracker::endRegion() on region that is not the current region: %s (current = %s)\n",
            tag.c_str(), region.tag.c_str());
        SST_Exit(1);
    }

    region.end_time = sst_get_cpu_time();
    region.end_mem  = Core::localMemSize();

    current_regions_.pop();

    // Barrier until all ranks are ready to collect total and max
    // utilizations
    SST_MPI_Barrier(MPI_COMM_WORLD);

    ////// Execution time //////
    mpi_double_int_t in, out;
    in.val  = region.end_time - region.begin_time;
    in.rank = rank_;

    SST_MPI_Allreduce(&in, &out, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);
    region.rollup_duration.push_back(out.val);
    region.rollup_max_duration_rank = out.rank;

    ///// Memory Usage //////

    /// Begin memory ///
    uint64_t global_mem = 0;
    SST_MPI_Allreduce(&region.begin_mem, &global_mem, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    region.rollup_begin_mem.push_back(global_mem);

    uint64_t max_mem = 0;
    SST_MPI_Allreduce(&region.begin_mem, &max_mem, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    region.rollup_begin_mem.push_back(max_mem);

    // Because MPI doesn't have a guaranteed 64-bit data type for the
    // MAX_LOC operation, we can't use it without defining a custom
    // data type.  To get the location, just have each rank see if
    // they are the max, and if they are, set the location to their
    // rank and do another allreduce.
    int my_location = (region.begin_mem == max_mem) ? rank_ : 0;
    int location    = my_location;
    SST_MPI_Allreduce(&my_location, &location, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    region.rollup_begin_mem_max_rank = location;

    /// End memory ///
    global_mem = 0;
    SST_MPI_Allreduce(&region.end_mem, &global_mem, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    region.rollup_end_mem.push_back(global_mem);

    max_mem = 0;
    SST_MPI_Allreduce(&region.end_mem, &max_mem, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    region.rollup_end_mem.push_back(max_mem);

    // Because MPI doesn't have a guaranteed 64-bit data type for the
    // MAX_LOC operation, we can't use it without defining a custom
    // data type.  To get the location, just have each rank see if
    // they are the max, and if they are, set the location to their
    // rank and do another allreduce.
    my_location = (region.end_mem == max_mem) ? rank_ : 0;
    location    = my_location;
    SST_MPI_Allreduce(&my_location, &location, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    region.rollup_end_mem_max_rank = location;
}


double
BasicPerfTracker::getRegionBeginTime(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getRegionBeginTime", false);
    return region.begin_time;
}

double
BasicPerfTracker::getRegionEndTime(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getRegionEndTime");
    return region.end_time;
}

double
BasicPerfTracker::getRegionDuration(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getRegionDuration");

    return region.end_time - region.begin_time;
}

double
BasicPerfTracker::getRegionGlobalDuration(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getRegionGlobalDuration");

    return region.rollup_duration[0];
}


uint64_t
BasicPerfTracker::getLocalRegionBeginMemSize(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getLocalRegionBeginMemSize");
    return region.begin_mem;
}

uint64_t
BasicPerfTracker::getGlobalTotalRegionBeginMemSize(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getGlobalTotalRegionBeginMemSize");
    return region.rollup_begin_mem[0];
}

std::pair<uint64_t, int>
BasicPerfTracker::getGlobalMaxRegionBeginMemSize(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getGlobalMaxRegionBeginMemSize");
    return std::make_pair(region.rollup_begin_mem[1], region.rollup_begin_mem_max_rank);
}


uint64_t
BasicPerfTracker::getLocalRegionEndMemSize(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getLocalRegionEndMemSize");
    return region.end_mem;
}

uint64_t
BasicPerfTracker::getGlobalTotalRegionEndMemSize(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getGlobalTotalRegionEndMemSize");
    return region.rollup_end_mem[0];
}

std::pair<uint64_t, int>
BasicPerfTracker::getGlobalMaxRegionEndMemSize(const std::string& tag)
{
    RegionPerfInfo& region = getRegion(tag, "getGlobalMaxRegionEndMemSize");
    return std::make_pair(region.rollup_end_mem[1], region.rollup_end_mem_max_rank);
}


void
BasicPerfTracker::addMetric(const std::string& name, uint64_t value)
{
    scalars_unsigned[name] = value;
}

void
BasicPerfTracker::addMetric(const std::string& name, int64_t value)
{
    scalars_signed[name] = value;
}

void
BasicPerfTracker::addMetric(const std::string& name, double value)
{
    scalars_float[name] = value;
}


uint64_t
BasicPerfTracker::getMetricUnsigned(const std::string& name)
{
    return scalars_unsigned[name];
}

int64_t
BasicPerfTracker::getMetricSigned(const std::string& name)
{
    return scalars_signed[name];
}

double
BasicPerfTracker::getMetricFloat(const std::string& name)
{
    return scalars_float[name];
}

// Need to use wstring for the unicode box drawing characters
std::wstring horizontal   = L"─"; // ─
std::wstring vertical     = L"│"; // │
std::wstring topLeft      = L"┌"; // ┌
std::wstring topRight     = L"┐"; // ┐
std::wstring bottomLeft   = L"└"; // └
std::wstring bottomRight  = L"┘"; // ┘
std::wstring vertAndRight = L"├"; // ├
std::wstring fullBox      = L"■"; // ■


void
BasicPerfTracker::outputRegionData(Output& out, size_t verbose)
{
    // We need to change the locale to support unicode.  Just need to
    // change the c-locale since we are using output, which uses
    // printf
    std::string orig_c_locale = std::setlocale(LC_ALL, nullptr);
    std::setlocale(LC_ALL, "en_US.UTF-8");

    bool print = (rank_ == 0);

    // Use vector as stack to keep track of hierarchy
    std::vector<std::pair<RegionPerfInfo*, bool>> region_stack;

    std::wstring prefix_stats;
    std::wstring prefix_region;

    for ( auto& x : regions_ ) {

        if ( x.level > verbose ) continue;

        // First, need to see if we need to remove the last region
        // and/or adjust the bool that controls the printing of the
        // vertical bars

        // If the level has descreased, need to pop all regions
        // that are greater than current level

        if ( region_stack.size() > 1 ) {
            while ( region_stack.back().first->level > x.level ) {
                region_stack.pop_back();
            }

            // If last region is same level as this region, remove it
            if ( region_stack.back().first->level == x.level ) {
                region_stack.pop_back();
            }
        }

        std::wstring region_indicator;
        if ( x.level == 1 ) {
            region_indicator += L"■ ";
        }
        else if ( !x.last_of_level ) {
            region_indicator += L"├ ■ ";
        }
        else {
            region_indicator += L"└ ■ ";
        }

        // If I'm the last child at this level, need to turn | off for my parent
        if ( x.level != 1 && x.last_of_level ) region_stack.back().second = false;

        // Create prefix string
        std::wstring prefix;
        for ( size_t i = 0; i < region_stack.size(); ++i ) {
            if ( region_stack[i].second )
                prefix += L"│ ";
            else
                prefix += L"  ";
        }

        // If this is the last level to print because of verbosity,
        // act like the region has no children
        bool has_child = x.has_child && x.level != verbose;
        region_stack.push_back(std::make_pair(&x, has_child));

        // The prefix for the region string needs to remove the indent
        // caused by the current level
        std::wstring region_prefix = prefix.substr(0, prefix.length() - 2);

        if ( print ) out.output("%ls%ls%s\n", region_prefix.c_str(), region_indicator.c_str(), x.tag.c_str());

        // Print the perf values
        std::wstring stat_prefix = prefix + (has_child ? L"│ " : L"  ") + L"├──";
        if ( print ) out.output("%ls Duration: %.3lf seconds\n", stat_prefix.c_str(), x.end_time - x.begin_time);

        uint64_t    mem_size_global = getGlobalTotalRegionEndMemSize(x.tag);
        UnitAlgebra mem_size_global_us("1kB");
        mem_size_global_us *= mem_size_global;

        stat_prefix = prefix + (has_child ? L"│ " : L"  ") + L"└──";
        if ( num_ranks_ > 1 ) {
            auto        mem_size_max = getGlobalMaxRegionEndMemSize(x.tag);
            UnitAlgebra mem_size_max_us("1kB");
            mem_size_max_us *= mem_size_max.first;

            if ( print )
                out.output("%ls Memory: Total - %s, Max - %s (rank %d)\n", stat_prefix.c_str(),
                    mem_size_global_us.toStringBestSI(4).c_str(), mem_size_max_us.toStringBestSI(4).c_str(),
                    mem_size_max.second);
        }
        else {
            if ( print )
                out.output(
                    "%ls Memory: Total - %s\n", stat_prefix.c_str(), mem_size_global_us.toStringBestSI(4).c_str());
        }
    }

    // Restore the locale
    std::setlocale(LC_ALL, orig_c_locale.c_str());
}

} // namespace SST::Util
