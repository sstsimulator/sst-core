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

#include "sst/core/profile/eventHandlerProfileTool.h"

#include "sst/core/output.h"
#include "sst/core/sst_types.h"

#include <chrono>

namespace SST {
namespace Profile {


EventHandlerProfileTool::EventHandlerProfileTool(ProfileToolId_t id, const std::string& name, Params& params) :
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
            CALL_INFO_LONG, 1, "ERROR: unsupported level specified for EventHandlerProfileTool: %s\n", level.c_str());
    }

    track_ports_ = params.find<bool>("track_ports", "false");

    profile_sends_    = params.find<bool>("profile_sends", "false");
    profile_receives_ = params.find<bool>("profile_receives", "true");
}

std::string
EventHandlerProfileTool::getKeyForHandler(const HandlerMetaData& mdata)
{
    const EventHandlerMetaData& data = dynamic_cast<const EventHandlerMetaData&>(mdata);

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

    if ( track_ports_ ) key = key + ":" + data.port_name;
    return key;
}

EventHandlerProfileToolCount::EventHandlerProfileToolCount(
    ProfileToolId_t id, const std::string& name, Params& params) :
    EventHandlerProfileTool(id, name, params)
{}

uintptr_t
EventHandlerProfileToolCount::registerHandler(const HandlerMetaData& mdata)
{
    return reinterpret_cast<uintptr_t>(&counts_[getKeyForHandler(mdata)]);
}

void
EventHandlerProfileToolCount::handlerStart(uintptr_t key)
{
    reinterpret_cast<event_data_t*>(key)->recv_count++;
}

void
EventHandlerProfileToolCount::eventSent(uintptr_t key, Event* UNUSED(ev))
{
    reinterpret_cast<event_data_t*>(key)->send_count++;
}


void
EventHandlerProfileToolCount::outputData(FILE* fp)
{
    fprintf(fp, "%s (id = %" PRIu64 ")\n", name.c_str(), my_id);
    fprintf(fp, "Name");
    if ( profile_receives_ ) fprintf(fp, ", recv count");
    if ( profile_sends_ ) fprintf(fp, ", send count");
    fprintf(fp, "\n");
    for ( auto& x : counts_ ) {
        fprintf(fp, "%s", x.first.c_str());
        if ( profile_receives_ ) fprintf(fp, ", %" PRIu64, x.second.recv_count);
        if ( profile_sends_ ) fprintf(fp, ", %" PRIu64, x.second.send_count);
        fprintf(fp, "\n");
    }
}


template <typename T>
EventHandlerProfileToolTime<T>::EventHandlerProfileToolTime(
    ProfileToolId_t id, const std::string& name, Params& params) :
    EventHandlerProfileTool(id, name, params)
{}

template <typename T>
uintptr_t
EventHandlerProfileToolTime<T>::registerHandler(const HandlerMetaData& mdata)
{
    return reinterpret_cast<uintptr_t>(&times_[getKeyForHandler(mdata)]);
}

template <typename T>
void
EventHandlerProfileToolTime<T>::outputData(FILE* fp)
{
    fprintf(fp, "%s (id = %" PRIu64 ")\n", name.c_str(), my_id);
    fprintf(fp, "Name");
    if ( profile_receives_ ) fprintf(fp, ", recv count, recv time (s)");
    if ( profile_sends_ ) fprintf(fp, ", send count");
    fprintf(fp, "\n");
    for ( auto& x : times_ ) {
        fprintf(fp, "%s", x.first.c_str());
        if ( profile_receives_ )
            fprintf(fp, ", %" PRIu64 ", %lf", x.second.recv_count, ((double)x.second.recv_time) / 1000000000.0);
        if ( profile_sends_ ) fprintf(fp, ", %" PRIu64, x.second.send_count);
        fprintf(fp, "\n");
    }
}


class EventHandlerProfileToolTimeHighResolution : public EventHandlerProfileToolTime<std::chrono::high_resolution_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        EventHandlerProfileToolTimeHighResolution,
        SST::Profile::EventHandlerProfileTool,
        "sst",
        "profile.handler.event.time.high_resolution",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time handlers using a high resolution clock"
    )

    EventHandlerProfileToolTimeHighResolution(ProfileToolId_t id, const std::string& name, Params& params) :
        EventHandlerProfileToolTime<std::chrono::high_resolution_clock>(id, name, params)
    {}

    ~EventHandlerProfileToolTimeHighResolution() {}

    SST_ELI_EXPORT(EventHandlerProfileToolTimeHighResolution)
};

class EventHandlerProfileToolTimeSteady : public EventHandlerProfileToolTime<std::chrono::steady_clock>
{
public:
    SST_ELI_REGISTER_PROFILETOOL(
        EventHandlerProfileToolTimeSteady,
        SST::Profile::EventHandlerProfileTool,
        "sst",
        "profile.handler.event.time.steady",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will time handlers using a steady clock"
    )

    EventHandlerProfileToolTimeSteady(ProfileToolId_t id, const std::string& name, Params& params) :
        EventHandlerProfileToolTime<std::chrono::steady_clock>(id, name, params)
    {}

    ~EventHandlerProfileToolTimeSteady() {}

    SST_ELI_EXPORT(EventHandlerProfileToolTimeSteady)
};


} // namespace Profile
} // namespace SST
