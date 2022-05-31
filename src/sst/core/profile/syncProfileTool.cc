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

#include "sst_config.h"

#include "sst/core/profile/syncProfileTool.h"

#include "sst/core/output.h"
#include "sst/core/sst_types.h"

#include <chrono>

namespace SST {
namespace Profile {


SyncProfileTool::SyncProfileTool(ProfileToolId_t id, const std::string& name, Params& UNUSED(params)) :
    ProfileTool(id, name)
{}

SyncProfileToolCount::SyncProfileToolCount(ProfileToolId_t id, const std::string& name, Params& params) :
    SyncProfileTool(id, name, params)
{}

void
SyncProfileToolCount::syncManagerStart()
{
    syncmanager_count++;
}


void
SyncProfileToolCount::outputData(FILE* fp)
{
    fprintf(fp, "%s (id = %" PRIu64 ")\n", name.c_str(), my_id);
    fprintf(fp, "  SyncManager Count = %" PRIu64 "\n", syncmanager_count);
}


template <typename T>
SyncProfileToolTime<T>::SyncProfileToolTime(ProfileToolId_t id, const std::string& name, Params& params) :
    SyncProfileTool(id, name, params)
{}

template <typename T>
void
SyncProfileToolTime<T>::outputData(FILE* fp)
{
    fprintf(fp, "%s (id = %" PRIu64 ")\n", name.c_str(), my_id);
    fprintf(fp, "  SyncManager Count = %" PRIu64 "\n", syncmanager_count);
    fprintf(fp, "  Total SyncManager Time = %lfs\n", (float)syncmanager_time / 1000000000.0);
    fprintf(fp, "  Average SyncManager Time = %" PRIu64 "ns\n", syncmanager_time / syncmanager_count);
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

    SyncProfileToolTimeHighResolution(ProfileToolId_t id, const std::string& name, Params& params) :
        SyncProfileToolTime<std::chrono::high_resolution_clock>(id, name, params)
    {}

    ~SyncProfileToolTimeHighResolution() {}

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

    SyncProfileToolTimeSteady(ProfileToolId_t id, const std::string& name, Params& params) :
        SyncProfileToolTime<std::chrono::steady_clock>(id, name, params)
    {}

    ~SyncProfileToolTimeSteady() {}

    SST_ELI_EXPORT(SyncProfileToolTimeSteady)
};


} // namespace Profile
} // namespace SST
