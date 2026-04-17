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

#include "sst/core/profile/syncProfileTool.h"

#include "sst/core/output.h"
#include "sst/core/sst_types.h"
#include "sst/core/util/perfReporter.h"

#include <chrono>
#include <cinttypes>
#include <cstdio>

namespace SST::Profile {

SyncProfileTool::SyncProfileTool(const std::string& name, Params& UNUSED(params)) :
    ProfileTool(name)
{}

SyncProfileToolCount::SyncProfileToolCount(const std::string& name, Params& params) :
    SyncProfileTool(name, params)
{}

void
SyncProfileToolCount::syncManagerStart(bool ranksync)
{
    if ( !ranksync ) {
        threadsync_count_++;
    }
    else {
        syncmanager_count++;
    }
}

void
SyncProfileToolCount::updateSyncSize(size_t bytes, size_t events)
{
    bytes_exchanged_ += bytes;
    events_exchanged_ += events;
}

void
SyncProfileToolCount::outputData(SST::Util::DataRecord* record, RankInfo rank)
{
    if ( rank.thread == 0 ) {
        record->setFormat(SST::Util::DataRecord::TextFormat::list);

        std::map<std::string, std::pair<std::string, std::string>> keys = { { "sync", { "Sync Profiling Data", "" } },
            { "ranksync_total_count", { "RankSync Total Syncs", "" } },
            { "threadsync_total_count", { "ThreadSync Total Syncs", "" } },
            { "ranksync_total_events", { "RankSync Total Events Exchanged", "" } },
            { "ranksync_total_data", { "RankSync Total Data Exchanged", "" } } };

        record->setKeys(keys);
    }
    record->addChild("rank" + std::to_string(rank.rank));
    record->addData("ranksync_total_count", syncmanager_count);
    record->addData("threadsync_total_count", threadsync_count_);
    record->addData("ranksync_total_events", events_exchanged_);
    record->addData("ranksync_total_data", UnitAlgebra(std::to_string(bytes_exchanged_) + "B"));
}


template <typename T>
SyncProfileToolTime<T>::SyncProfileToolTime(const std::string& name, Params& params) :
    SyncProfileTool(name, params)
{}

template <typename T>
void
SyncProfileToolTime<T>::outputData(SST::Util::DataRecord* record, RankInfo rank)
{
    if ( rank.thread == 0 ) {
        std::map<std::string, std::pair<std::string, std::string>> keys = { { "sync", { "Sync Profiling Data", "" } },
            { "ranksync_total_count", { "RankSync Total Syncs", "" } },
            { "ranksync_total_time", { "RankSync Total Time", "" } },
            { "ranksync_avg_time", { "RankSync Time per Sync", "" } },
            { "threadsync_total_count", { "ThreadSync Total Syncs", "" } },
            { "threadsync_total_time", { "ThreadSync Total Time", "" } },
            { "threadsync_avg_time", { "ThreadSync Time per Sync", "" } },
            { "ranksync_total_events", { "RankSync Total Events Exchanged", "" } },
            { "ranksync_total_data", { "RankSync Total Data Exchanged", "" } } };

        record->setKeys(keys);
        record->setFormat(SST::Util::DataRecord::TextFormat::list);
    }
    record->addChild("rank" + std::to_string(rank.rank));
    record->addData("ranksync_total_count", syncmanager_count);
    record->addData("ranksync_total_time", UnitAlgebra(std::to_string((float)syncmanager_time / 1000000000.0) + "s"));
    record->addData("threadsync_total_count", threadsync_count);
    record->addData("threadsync_total_time", UnitAlgebra(std::to_string((float)threadsync_time / 1000000000.0) + "s"));

    if ( syncmanager_count != 0 ) {
        record->addData("ranksync_avg_time", UnitAlgebra(std::to_string(syncmanager_time / syncmanager_count) + "ns"));
    }
    else {
        record->addData("ranksync_avg_time", UnitAlgebra("0ns"));
    }
    if ( threadsync_count != 0 ) {
        record->addData("threadsync_avg_time", UnitAlgebra(std::to_string(threadsync_time / threadsync_count) + "ns"));
    }
    else {
        record->addData("threadsync_avg_time", UnitAlgebra("0ns"));
    }
    record->addData("ranksync_total_events", events_exchanged_);
    record->addData("ranksync_total_data", UnitAlgebra(std::to_string(bytes_exchanged_) + "B"));
}


class SyncProfileToolTimeHighResolution : public SyncProfileToolTime<std::chrono::high_resolution_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        SyncProfileToolTimeHighResolution,
        SST::Profile::SyncProfileTool,
        "sst",
        "profile.sync.time.high_resolution",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time handlers using a high resolution clock"
    )

    SyncProfileToolTimeHighResolution(const std::string& name, Params& params) :
        SyncProfileToolTime<std::chrono::high_resolution_clock>(name, params)
    {}

    ~SyncProfileToolTimeHighResolution() = default;

    SST_ELI_EXPORT(SyncProfileToolTimeHighResolution)
};

class SyncProfileToolTimeSteady : public SyncProfileToolTime<std::chrono::steady_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        SyncProfileToolTimeSteady,
        SST::Profile::SyncProfileTool,
        "sst",
        "profile.sync.time.steady",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time handlers using a steady clock"
    )

    SyncProfileToolTimeSteady(const std::string& name, Params& params) :
        SyncProfileToolTime<std::chrono::steady_clock>(name, params)
    {}

    ~SyncProfileToolTimeSteady() = default;

    SST_ELI_EXPORT(SyncProfileToolTimeSteady)
};


} // namespace SST::Profile
