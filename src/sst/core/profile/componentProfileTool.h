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

#ifndef SST_CORE_PROFILE_COMPONENTPROFILETOOL_H
#define SST_CORE_PROFILE_COMPONENTPROFILETOOL_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/profile/profiletool.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <chrono>
#include <map>

namespace SST {

class BaseComponent;

namespace Profile {

// Base class for profiling tools designed to profile in Components
// and SubComponents. For these types of profiling tools, you can
// trace at various levels:

// 1 - Global: all profiling will be consolidated into one global
// profile

// 2 - Type: all profiling will be consolidated into one profile per
// (Sub)Component type.

// 3 - Component: profiling will be consolidated at the Component
// level and all SubComponent data will be consolidated with its
// parent component

// 4 - SubComponent: profiling will be consolidated at the
// SubComponent level
class ComponentProfileTool : public ProfileTool
{
public:
    SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(SST::Profile::ComponentProfileTool, SST::Profile::ProfileTool, Params&)

    SST_ELI_DOCUMENT_PARAMS(
        { "level", "Level at which to track profile (global, type, component, subcomponent)", "type" },
        { "track_points", "Determines whether independent profiling points are tracked", "true" },
    )

    enum class Profile_Level { Global, Type, Component, Subcomponent };

    ComponentProfileTool(const std::string& name, Params& params);


    virtual uintptr_t registerProfilePoint(
        const std::string& point, ComponentId_t id, const std::string& name, const std::string& type) = 0;
    std::string
    getKeyForCodeSegment(const std::string& point, ComponentId_t id, const std::string& name, const std::string& type);

protected:
    Profile_Level profile_level_;

private:
    bool track_points_;
};


// Profiler API designed to profile code segments in Components and
// SubComponents
class ComponentCodeSegmentProfileTool : public ComponentProfileTool
{
    friend class BaseComponent;

public:
    SST_ELI_REGISTER_PROFILETOOL_DERIVED_API(SST::Profile::ComponentCodeSegmentProfileTool, SST::Profile::ComponentProfileTool, Params&)

    ComponentCodeSegmentProfileTool(const std::string& name, Params& params);

    virtual void codeSegmentStart(uintptr_t UNUSED(key)) {}
    virtual void codeSegmentEnd(uintptr_t UNUSED(key)) {}

    class ProfilePoint
    {
    public:
        ProfilePoint() {}
        ~ProfilePoint() {}

        inline void codeSegmentStart()
        {
            for ( auto& x : tools ) {
                x.first->codeSegmentStart(x.second);
            }
        }
        inline void codeSegmentEnd()
        {
            for ( auto& x : tools ) {
                x.first->codeSegmentEnd(x.second);
            }
        }

        void registerProfilePoint(
            ComponentCodeSegmentProfileTool* tool, const std::string& point, ComponentId_t id, const std::string& name,
            const std::string& type)
        {
            uintptr_t key = tool->registerProfilePoint(point, id, name, type);
            tools.push_back(std::make_pair(tool, key));
        }

    private:
        std::vector<std::pair<ComponentCodeSegmentProfileTool*, uintptr_t>> tools;
    };
};

/**
   Profile tool that will count the number of times a handler is
   called
 */
class ComponentCodeSegmentProfileToolCount : public ComponentCodeSegmentProfileTool
{

public:
    SST_ELI_REGISTER_PROFILETOOL(
        ComponentCodeSegmentProfileToolCount,
        SST::Profile::ComponentCodeSegmentProfileTool,
        "sst",
        "profile.component.codesegment.count",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Profiler that will count times through a marked code segment"
    )

    ComponentCodeSegmentProfileToolCount(const std::string& name, Params& params);

    virtual ~ComponentCodeSegmentProfileToolCount() {}

    uintptr_t registerProfilePoint(
        const std::string& point, ComponentId_t id, const std::string& name, const std::string& type) override;

    void codeSegmentStart(uintptr_t key) override;

    void outputData(FILE* fp) override;

private:
    std::map<std::string, uint64_t> counts_;
};

/**
   Profile tool that will count the number of times a handler is
   called
 */
template <typename T>
class ComponentCodeSegmentProfileToolTime : public ComponentCodeSegmentProfileTool
{
    struct segment_data_t
    {
        uint64_t time;
        uint64_t count;

        segment_data_t() : time(0), count(0) {}
    };

public:
    ComponentCodeSegmentProfileToolTime(const std::string& name, Params& params);

    virtual ~ComponentCodeSegmentProfileToolTime() {}

    uintptr_t registerProfilePoint(
        const std::string& point, ComponentId_t id, const std::string& name, const std::string& type) override;

    void codeSegmentStart(uintptr_t UNUSED(key)) override { start_time_ = T::now(); }

    void codeSegmentEnd(uintptr_t key) override
    {
        auto            total_time = T::now() - start_time_;
        segment_data_t* entry      = reinterpret_cast<segment_data_t*>(key);
        entry->time += std::chrono::duration_cast<std::chrono::nanoseconds>(total_time).count();
        entry->count++;
    }

    void outputData(FILE* fp) override;

private:
    typename T::time_point                start_time_;
    std::map<std::string, segment_data_t> times_;
};

} // namespace Profile
} // namespace SST

#endif // SST_CORE_PROFILE_COMPONENTPROFILETOOL_H
