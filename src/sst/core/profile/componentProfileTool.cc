// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/profile/componentProfileTool.h"

#include "sst/core/output.h"
#include "sst/core/sst_types.h"

#include <chrono>

namespace SST {
namespace Profile {


ComponentProfileTool::ComponentProfileTool(const std::string& name, Params& params) : ProfileTool(name)
{
    std::string level = params.find<std::string>("level", "type");
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
            CALL_INFO_LONG, 1, "ERROR: unsupported level specified for ComponentProfileTool: %s\n", level.c_str());
    }
    track_points_ = params.find<bool>("track_points", "true");
}

std::string
ComponentProfileTool::getKeyForCodeSegment(
    const std::string& point, ComponentId_t UNUSED(id), const std::string& name, const std::string& type)
{
    std::string key;
    switch ( profile_level_ ) {
    case Profile_Level::Global:
        key = "global";
        break;
    case Profile_Level::Type:
        key = type;
        break;
    case Profile_Level::Component:
        // Get just the component name, no subcomponents
        key = name.substr(0, name.find(":"));
        break;
    case Profile_Level::Subcomponent:
        key = name;
        break;
    default:
        break;
    }

    if ( track_points_ ) key = key + "/" + point;
    return key;
}


ComponentCodeSegmentProfileTool::ComponentCodeSegmentProfileTool(const std::string& name, Params& params) :
    ComponentProfileTool(name, params)
{}


ComponentCodeSegmentProfileToolCount::ComponentCodeSegmentProfileToolCount(const std::string& name, Params& params) :
    ComponentCodeSegmentProfileTool(name, params)
{}

uintptr_t
ComponentCodeSegmentProfileToolCount::registerProfilePoint(
    const std::string& point, ComponentId_t id, const std::string& name, const std::string& type)
{
    return reinterpret_cast<uintptr_t>(&counts_[getKeyForCodeSegment(point, id, name, type)]);
}

void
ComponentCodeSegmentProfileToolCount::codeSegmentStart(uintptr_t key)
{
    (*reinterpret_cast<uint64_t*>(key))++;
}

void
ComponentCodeSegmentProfileToolCount::outputData(FILE* fp)
{
    fprintf(fp, "%s\n", name.c_str());
    fprintf(fp, "Name, Count\n");
    for ( auto& x : counts_ ) {
        fprintf(fp, "%s", x.first.c_str());
        fprintf(fp, ", %" PRIu64 "\n", x.second);
    }
}


template <typename T>
ComponentCodeSegmentProfileToolTime<T>::ComponentCodeSegmentProfileToolTime(const std::string& name, Params& params) :
    ComponentCodeSegmentProfileTool(name, params)
{}

template <typename T>
uintptr_t
ComponentCodeSegmentProfileToolTime<T>::registerProfilePoint(
    const std::string& point, ComponentId_t id, const std::string& name, const std::string& type)
{
    return reinterpret_cast<uintptr_t>(&times_[getKeyForCodeSegment(point, id, name, type)]);
}

template <typename T>
void
ComponentCodeSegmentProfileToolTime<T>::outputData(FILE* fp)
{
    fprintf(fp, "%s\n", name.c_str());
    fprintf(fp, "Name, count, time (s), avg time (ns)\n");
    for ( auto& x : times_ ) {
        fprintf(fp, "%s", x.first.c_str());
        fprintf(
            fp, ", %" PRIu64 ", %lf, %" PRIu64 "\n", x.second.count, ((double)x.second.time) / 1000000000.0,
            x.second.count == 0 ? 0 : x.second.time / x.second.count);
    }
}


class ComponentCodeSegmentProfileToolTimeHighResolution :
    public ComponentCodeSegmentProfileToolTime<std::chrono::high_resolution_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        ComponentCodeSegmentProfileToolTimeHighResolution,
        SST::Profile::ComponentCodeSegmentProfileTool,
        "sst",
        "profile.component.codesegment.time.high_resolution",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time component code segments using a high resolution clock"
    )

    ComponentCodeSegmentProfileToolTimeHighResolution(const std::string& name, Params& params) :
        ComponentCodeSegmentProfileToolTime<std::chrono::high_resolution_clock>(name, params)
    {}

    ~ComponentCodeSegmentProfileToolTimeHighResolution() {}

    SST_ELI_EXPORT(ComponentCodeSegmentProfileToolTimeHighResolution)
};

class ComponentCodeSegmentProfileToolTimeSteady : public ComponentCodeSegmentProfileToolTime<std::chrono::steady_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        ComponentCodeSegmentProfileToolTimeSteady,
        SST::Profile::ComponentCodeSegmentProfileTool,
        "sst",
        "profile.component.codesegment.time.steady",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time component code segments using a steady clock"
    )

    ComponentCodeSegmentProfileToolTimeSteady(const std::string& name, Params& params) :
        ComponentCodeSegmentProfileToolTime<std::chrono::steady_clock>(name, params)
    {}

    ~ComponentCodeSegmentProfileToolTimeSteady() {}

    SST_ELI_EXPORT(ComponentCodeSegmentProfileToolTimeSteady)
};


} // namespace Profile
} // namespace SST
