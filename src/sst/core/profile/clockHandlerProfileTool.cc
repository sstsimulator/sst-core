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

#include "sst/core/profile/clockHandlerProfileTool.h"

#include "sst/core/output.h"
#include "sst/core/sst_types.h"

#include <chrono>

namespace SST {
namespace Profile {


ClockHandlerProfileTool::ClockHandlerProfileTool(ProfileToolId_t id, const std::string& name, Params& params) :
    HandlerProfileToolAPI(id, name)
{
    std::string level = params.find<std::string>("level", "component");
    if ( level == "global" )
        profile_level_ = Profile_Level::Global;
    else if ( level == "type" )
        profile_level_ = Profile_Level::Type;
    else if ( level == "component" )
        profile_level_ = Profile_Level::Component;
    else if ( level == "subcomponent" )
        profile_level_ = Profile_Level::Subcomponent;
    else {
        Output::getDefaultObject().fatal(
            CALL_INFO_LONG, 1, "ERROR: unsupported level specified for ClockHandlerProfileTool: %s\n", level.c_str());
    }
}

std::string
ClockHandlerProfileTool::getKeyForHandler(const HandlerMetaData& mdata)
{
    const ClockHandlerMetaData& data = dynamic_cast<const ClockHandlerMetaData&>(mdata);

    std::string key;
    switch ( profile_level_ ) {
    case Profile_Level::Global:
        key = "global";
        break;
    case Profile_Level::Type:
        key = data.comp_type;
        break;
    case Profile_Level::Component:
        // Get just the component name, no subcomponents
        key = data.comp_name.substr(0, data.comp_name.find(":"));
        break;
    case Profile_Level::Subcomponent:
        key = data.comp_name;
        break;
    default:
        break;
    }
    return key;
}

ClockHandlerProfileToolCount::ClockHandlerProfileToolCount(
    ProfileToolId_t id, const std::string& name, Params& params) :
    ClockHandlerProfileTool(id, name, params)
{}

uintptr_t
ClockHandlerProfileToolCount::registerHandler(const HandlerMetaData& mdata)
{
    return reinterpret_cast<uintptr_t>(&counts_[getKeyForHandler(mdata)]);
}

void
ClockHandlerProfileToolCount::handlerStart(uintptr_t key)
{
    (*reinterpret_cast<uint64_t*>(key))++;
}


void
ClockHandlerProfileToolCount::outputData(FILE* fp)
{
    fprintf(fp, "%s (id = %" PRIu64 ")\n", name.c_str(), my_id);
    fprintf(fp, "Name, count\n");
    for ( auto& x : counts_ ) {
        fprintf(fp, "%s, %" PRIu64 "\n", x.first.c_str(), x.second);
    }
}


template <typename T>
ClockHandlerProfileToolTime<T>::ClockHandlerProfileToolTime(
    ProfileToolId_t id, const std::string& name, Params& params) :
    ClockHandlerProfileTool(id, name, params)
{}

template <typename T>
uintptr_t
ClockHandlerProfileToolTime<T>::registerHandler(const HandlerMetaData& mdata)
{
    return reinterpret_cast<uintptr_t>(&times_[getKeyForHandler(mdata)]);
}

template <typename T>
void
ClockHandlerProfileToolTime<T>::outputData(FILE* fp)
{
    fprintf(fp, "%s (id = %" PRIu64 ")\n", name.c_str(), my_id);
    fprintf(fp, "Name, count, handler time (s), avg. handler time (ns)\n");
    for ( auto& x : times_ ) {
        fprintf(
            fp, "%s, %" PRIu64 ", %lf, %" PRIu64 "\n", x.first.c_str(), x.second.count,
            ((double)x.second.time) / 1000000000.0, x.second.count == 0 ? 0 : x.second.time / x.second.count);
    }
}


class ClockHandlerProfileToolTimeHighResolution : public ClockHandlerProfileToolTime<std::chrono::high_resolution_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        ClockHandlerProfileToolTimeHighResolution,
        SST::Profile::ClockHandlerProfileTool,
        "sst",
        "profile.handler.clock.time.high_resolution",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time handlers using a high resolution clock"
    )

    ClockHandlerProfileToolTimeHighResolution(ProfileToolId_t id, const std::string& name, Params& params) :
        ClockHandlerProfileToolTime<std::chrono::high_resolution_clock>(id, name, params)
    {}

    ~ClockHandlerProfileToolTimeHighResolution() {}

    SST_ELI_EXPORT(ClockHandlerProfileToolTimeHighResolution)
};

class ClockHandlerProfileToolTimeSteady : public ClockHandlerProfileToolTime<std::chrono::steady_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        ClockHandlerProfileToolTimeSteady,
        SST::Profile::ClockHandlerProfileTool,
        "sst",
        "profile.handler.clock.time.steady",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time handlers using a steady clock"
    )

    ClockHandlerProfileToolTimeSteady(ProfileToolId_t id, const std::string& name, Params& params) :
        ClockHandlerProfileToolTime<std::chrono::steady_clock>(id, name, params)
    {}

    ~ClockHandlerProfileToolTimeSteady() {}

    SST_ELI_EXPORT(ClockHandlerProfileToolTimeSteady)
};


} // namespace Profile
} // namespace SST
