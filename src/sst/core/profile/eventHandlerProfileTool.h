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

#ifndef SST_CORE_PROFILE_EVENTHANDLERPROFILETOOL_H
#define SST_CORE_PROFILE_EVENTHANDLERPROFILETOOL_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/event.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/warnmacros.h"

#include <chrono>
#include <map>

namespace SST {

namespace Profile {


class EventHandlerProfileTool : public HandlerProfileToolAPI
{
public:
    SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(SST::Profile::EventHandlerProfileTool, SST::HandlerProfileToolAPI, Params&)

    SST_ELI_DOCUMENT_PARAMS(
        { "level", "Level at which to track profile (global, type, component, subcomponent)", "type" },
        { "track_ports",  "Controls whether to track by individual ports", "false" },
        { "profile_sends",  "Controls whether sends are profiled (due to location of profiling point in the code, turning on send profiling will incur relatively high overhead)", "false" },
        { "profile_receives",  "Controls whether receives are profiled", "true" },
    )

    enum class Profile_Level { Global, Type, Component, Subcomponent };

    EventHandlerProfileTool(const std::string& name, Params& params);

    virtual void eventSent(uintptr_t UNUSED(key), Event* UNUSED(ev)) {}

    bool profileSends() { return profile_sends_; }
    bool profileReceives() { return profile_receives_; }

protected:
    std::string getKeyForHandler(const HandlerMetaData& mdata);

    Profile_Level profile_level_;
    bool          track_ports_;

    bool profile_sends_;
    bool profile_receives_;
};


/**
   Profile tool that will count the number of times a handler is
   called
 */
class EventHandlerProfileToolCount : public EventHandlerProfileTool
{
    struct event_data_t
    {
        uint64_t recv_count;
        uint64_t send_count;

        event_data_t() : recv_count(0), send_count(0) {}
    };

public:
    SST_ELI_REGISTER_PROFILETOOL(
        EventHandlerProfileToolCount,
        SST::Profile::EventHandlerProfileTool,
        "sst",
        "profile.handler.event.count",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will count calls to handler functions"
    )

    EventHandlerProfileToolCount(const std::string& name, Params& params);

    virtual ~EventHandlerProfileToolCount() {}

    uintptr_t registerHandler(const HandlerMetaData& mdata) override;

    void handlerStart(uintptr_t key) override;
    void eventSent(uintptr_t UNUSED(key), Event* UNUSED(ev)) override;

    void outputData(FILE* fp) override;

private:
    std::map<std::string, event_data_t> counts_;
};

/**
   Profile tool that will count the number of times a handler is
   called
 */
template <typename T>
class EventHandlerProfileToolTime : public EventHandlerProfileTool
{
    struct event_data_t
    {
        uint64_t recv_time;
        uint64_t recv_count;
        uint64_t send_count;

        event_data_t() : recv_time(0), recv_count(0), send_count(0) {}
    };

public:
    EventHandlerProfileToolTime(const std::string& name, Params& params);

    virtual ~EventHandlerProfileToolTime() {}

    uintptr_t registerHandler(const HandlerMetaData& mdata) override;

    void handlerStart(uintptr_t UNUSED(key)) override { start_time_ = T::now(); }

    void handlerEnd(uintptr_t key) override
    {
        auto          total_time = T::now() - start_time_;
        event_data_t* entry      = reinterpret_cast<event_data_t*>(key);
        entry->recv_time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_time).count();
        entry->recv_count++;
    }

    void eventSent(uintptr_t key, Event* UNUSED(ev)) override { reinterpret_cast<event_data_t*>(key)->send_count++; }

    void outputData(FILE* fp) override;

private:
    typename T::time_point              start_time_;
    std::map<std::string, event_data_t> times_;
};

} // namespace Profile
} // namespace SST

#endif // SST_CORE_PROFILE_EVENTHANDLERPROFILETOOL_H
