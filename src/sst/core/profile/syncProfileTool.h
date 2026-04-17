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

#ifndef SST_CORE_PROFILE_SYNCPROFILETOOL_H
#define SST_CORE_PROFILE_SYNCPROFILETOOL_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/profile/profiletool.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/warnmacros.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <string>

namespace SST::Util {
class DataRecord;
}

namespace SST::Profile {

// Initial version of sync profiling tool.  The API is not yet complete.
class SyncProfileTool : public ProfileTool
{
public:
    SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(SST::Profile::SyncProfileTool, SST::Profile::ProfileTool, Params&)

    // Still to enable the level param in the class
    SST_ELI_DOCUMENT_PARAMS(
        // { "level", "Level at which to track profile (global, type, component, subcomponent)", "component" },
    )
    // enum class Profile_Level { Global, Type, Component, Subcomponent };

    SyncProfileTool(const std::string& name, Params& params);

    virtual void syncManagerStart(bool UNUSED(ranksync)) {}
    virtual void syncManagerEnd() {}

    virtual void updateSyncSize(size_t UNUSED(bytes), size_t UNUSED(events)) {}
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

    SyncProfileToolCount(const std::string& name, Params& params);

    virtual ~SyncProfileToolCount() {}

    void syncManagerStart(bool ranksync) override;

    void updateSyncSize(size_t bytes, size_t events) override;

    void outputData(SST::Util::DataRecord* record, RankInfo rank) override;

private:
    uint64_t syncmanager_count = 0;
    uint64_t threadsync_count_ = 0;
    uint64_t events_exchanged_ = 0;
    uint64_t bytes_exchanged_  = 0;
};

/**
   Profile tool that will count the number of times a handler is
   called
 */
template <typename T>
class SyncProfileToolTime : public SyncProfileTool
{
public:
    SyncProfileToolTime(const std::string& name, Params& params);

    virtual ~SyncProfileToolTime() {}

    void syncManagerStart(bool ranksync) override
    {
        start_time_ = T::now();
        rank_sync_  = ranksync;
    }

    void syncManagerEnd() override
    {
        auto total_time = T::now() - start_time_;

        if ( !rank_sync_ ) {
            threadsync_count++;
            threadsync_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_time).count();
        }
        else {
            syncmanager_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_time).count();
            syncmanager_count++;
        }
    }

    void updateSyncSize(size_t bytes, size_t events) override
    {
        bytes_exchanged_ += bytes;
        events_exchanged_ += events;
    }

    void outputData(SST::Util::DataRecord* record, RankInfo rank) override;

private:
    uint64_t syncmanager_time  = 0;
    uint64_t syncmanager_count = 0;
    uint64_t threadsync_count  = 0;
    uint64_t threadsync_time   = 0;
    uint64_t events_exchanged_ = 0;
    uint64_t bytes_exchanged_  = 0;
    bool     rank_sync_        = false;

    typename T::time_point start_time_;
};

// Class used to hold the list of profile tools installed in the SyncManager
// Here so that it can be used by multiple classes throughout the sync objects
class SyncProfileToolList
{
public:
    SyncProfileToolList() = default;

    void syncManagerStart(bool ranksync)
    {
        for ( auto* x : tools )
            x->syncManagerStart(ranksync);
    }

    void syncManagerEnd()
    {
        for ( auto* x : tools )
            x->syncManagerEnd();
    }

    void updateSyncSize(size_t bytes, size_t events)
    {
        for ( auto* x : tools )
            x->updateSyncSize(bytes, events);
    }

    /**
       Adds a profile tool the the list and registers this handler
       with the profile tool
    */
    void addProfileTool(Profile::SyncProfileTool* tool) { tools.push_back(tool); }

private:
    std::vector<Profile::SyncProfileTool*> tools;
};

} // namespace SST::Profile

#endif // SST_CORE_PROFILE_SYNCPROFILETOOL_H
