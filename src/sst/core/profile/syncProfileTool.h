// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_PROFILE_SYNCPROFILETOOL_H
#define SST_CORE_PROFILE_SYNCPROFILETOOL_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/warnmacros.h"

#include <chrono>
#include <map>

namespace SST {

namespace Profile {

// Initial version of sync profiling tool.  The API is not yet complete.
class SyncProfileTool : public ProfileTool
{
public:
    SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(SST::Profile::SyncProfileTool, SST::Profile::ProfileTool, Params&)

    SST_ELI_DOCUMENT_PARAMS(
        // { "level", "Level at which to track profile (global, type, component, subcomponent)", "component" },
        // { "track_ports",  "Controls whether to track by individual ports", "false" },
        // { "profile_sends",  "Controls whether sends are profiled (due to location of profiling point in the code, turning on send profiling will incur relatively high overhead)", "false" },
        // { "profile_receives",  "Controls whether receives are profiled", "true" },
    )

    // enum class Profile_Level { Global, Type, Component, Subcomponent };

    SyncProfileTool(ProfileToolId_t id, const std::string& name, Params& params);

    virtual void syncManagerStart() {}
    virtual void syncManagerEnd() {}
};


/**
   Profile tool that will count the number of times a handler is
   called
 */
class SyncProfileToolCount : public SyncProfileTool
{

public:
    SST_ELI_REGISTER_PROFILETOOL(
        SyncProfileToolCount,
        SST::Profile::SyncProfileTool,
        "sst",
        "profile.sync.count",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will count calls to sync"
    )

    SyncProfileToolCount(ProfileToolId_t id, const std::string& name, Params& params);

    virtual ~SyncProfileToolCount() {}

    void syncManagerStart() override;

    void outputData(FILE* fp) override;

private:
    uint64_t syncmanager_count = 0;
};

/**
   Profile tool that will count the number of times a handler is
   called
 */
template <typename T>
class SyncProfileToolTime : public SyncProfileTool
{
public:
    SyncProfileToolTime(ProfileToolId_t id, const std::string& name, Params& params);

    virtual ~SyncProfileToolTime() {}

    void syncManagerStart() override { start_time_ = T::now(); }

    void syncManagerEnd() override
    {
        auto total_time = T::now() - start_time_;
        syncmanager_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_time).count();
        syncmanager_count++;
    }

    void outputData(FILE* fp) override;

private:
    uint64_t syncmanager_time  = 0;
    uint64_t syncmanager_count = 0;

    typename T::time_point start_time_;
};

} // namespace Profile
} // namespace SST

#endif // SST_CORE_PROFILE_SYNCPROFILETOOL_H
