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

#ifndef SST_CORE_PROFILE_CLOCKHANDLERPROFILETOOL_H
#define SST_CORE_PROFILE_CLOCKHANDLERPROFILETOOL_H

#include "sst/core/clock.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/profile/profiletool.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/warnmacros.h"

#include <chrono>
#include <map>

namespace SST::Profile {

class ClockHandlerProfileTool : public ProfileTool, public Clock::HandlerBase::AttachPoint
{
public:
    SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(SST::Profile::ClockHandlerProfileTool, SST::Profile::ProfileTool, Params&)

    SST_ELI_DOCUMENT_PARAMS(
        { "level", "Level at which to track profile (global, type, component, subcomponent)", "type" },
    )

    enum class Profile_Level { Global, Type, Component, Subcomponent };

    ClockHandlerProfileTool(const std::string& name, Params& params);

    // Default implementations of attach point functions for profile
    // tools that don't use them
    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override {}
    void afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(remove)) override {}

protected:
    std::string getKeyForHandler(const AttachPointMetaData& mdata);

    Profile_Level profile_level_;
};


/**
   Profile tool that will count the number of times a handler is
   called
 */
class ClockHandlerProfileToolCount : public ClockHandlerProfileTool
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        ClockHandlerProfileToolCount,
        SST::Profile::ClockHandlerProfileTool,
        "sst",
        "profile.handler.clock.count",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will count calls to handler functions"
    )

    ClockHandlerProfileToolCount(const std::string& name, Params& params);

    virtual ~ClockHandlerProfileToolCount() {}

    uintptr_t registerHandler(const AttachPointMetaData& mdata) override;

    void beforeHandler(uintptr_t key, const Cycle_t& cycle) override;

    void outputData(FILE* fp) override;

private:
    std::map<std::string, uint64_t> counts_;
};

/**
   Profile tool that will count the number of times a handler is
   called
 */
template <typename T>
class ClockHandlerProfileToolTime : public ClockHandlerProfileTool
{
    struct clock_data_t
    {
        uint64_t time;
        uint64_t count;

        clock_data_t() : time(0), count(0) {}
    };

public:
    ClockHandlerProfileToolTime(const std::string& name, Params& params);

    virtual ~ClockHandlerProfileToolTime() {}

    uintptr_t registerHandler(const AttachPointMetaData& mdata) override;

    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override { start_time_ = T::now(); }

    void afterHandler(uintptr_t key, const bool& UNUSED(remove)) override
    {
        auto          total_time = T::now() - start_time_;
        clock_data_t* entry      = reinterpret_cast<clock_data_t*>(key);
        entry->time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_time).count();
        entry->count++;
    }

    void outputData(FILE* fp) override;

private:
    typename T::time_point              start_time_;
    std::map<std::string, clock_data_t> times_;
};

} // namespace SST::Profile

#endif // SST_CORE_PROFILE_CLOCKHANDLERPROFILETOOL_H
