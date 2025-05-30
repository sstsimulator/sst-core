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

#ifndef SST_CORE_BASECOMPONENT_H
#define SST_CORE_BASECOMPONENT_H

#include "sst/core/clock.h"
#include "sst/core/componentInfo.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/event.h"
#include "sst/core/factory.h"
#include "sst/core/portModule.h"
#include "sst/core/profile/componentProfileTool.h"
#include "sst/core/serialization/serializable_base.h"
#include "sst/core/serialization/serialize.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/warnmacros.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

using namespace SST::Statistics;

namespace SST {

class Component;
class ComponentExtension;
class Clock;
class Link;
class LinkMap;
class Module;
class Params;
class Simulation;
class Simulation_impl;
class SubComponent;
class SubComponentSlotInfo;
class TimeConverter;
class UnitAlgebra;
class WatchPoint;

namespace Core::Serialization::pvt {
class SerializeBaseComponentHelper;
}

/**
 * Main component object for the simulation.
 */
class BaseComponent : public SST::Core::Serialization::serializable_base
{

    friend class Component;
    friend class ComponentExtension;
    friend class ComponentInfo;
    friend class SubComponent;
    friend class SubComponentSlotInfo;

protected:
    using StatCreateFunction =
        std::function<Statistics::StatisticBase*(BaseComponent*, Statistics::StatisticProcessingEngine*,
            const std::string& /*type*/, const std::string& /*name*/, const std::string& /*subId*/, Params&)>;

    BaseComponent() = default; // For serialization only

public:
    explicit BaseComponent(ComponentId_t id);
    virtual ~BaseComponent();

    BaseComponent(const BaseComponent&)            = delete;
    BaseComponent& operator=(const BaseComponent&) = delete;

    const std::string& getType() const { return my_info_->getType(); }

    /** Returns unique component ID */
    inline ComponentId_t getId() const { return my_info_->id_; }

    /** Returns Component Statistic load level */
    inline uint8_t getStatisticLoadLevel() const { return my_info_->statLoadLevel; }

    /** Called when SIGINT or SIGTERM has been seen.
     * Allows components opportunity to clean up external state.
     */
    virtual void emergencyShutdown() {}

    /** Returns Component/SubComponent Name */
    inline const std::string& getName() const { return my_info_->getName(); }

    /** Returns the name of the parent Component, or, if called on a
     * Component, the name of that Component. */
    inline const std::string& getParentComponentName() const { return my_info_->getParentComponentName(); }

    /** Used during the init phase.  The method will be called each
     phase of initialization.  Initialization ends when no components
     have sent any data. */
    virtual void init(unsigned int UNUSED(phase)) {}
    /** Used during the complete phase after the end of simulation.
     The method will be called each phase of complete. Complete phase
     ends when no components have sent any data. */
    virtual void complete(unsigned int UNUSED(phase)) {}
    /** Called after all components have been constructed and
    initialization has completed, but before simulation time has
    begun. */
    virtual void setup() {}
    /** Called after complete phase, but before objects are
     destroyed. A good place to print out statistics. */
    virtual void finish() {}

    /** Currently unused function */
    virtual bool Status() { return 0; }

    /**
     * Called by the Simulation to request that the component
     * print it's current status.  Useful for debugging.
     * @param out The Output class which should be used to print component status.
     */
    virtual void printStatus(Output& UNUSED(out)) { return; }

    /** Get the core timebase */
    UnitAlgebra getCoreTimeBase() const;
    /** Return the current simulation time as a cycle count*/
    SimTime_t   getCurrentSimCycle() const;
    /** Return the current priority */
    int         getCurrentPriority() const;
    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;
    /** Return the end simulation time as a cycle count*/
    SimTime_t   getEndSimCycle() const;
    /** Return the end simulation time as a time */
    UnitAlgebra getEndSimTime() const;
    /** Get this instance's parallel rank */
    RankInfo    getRank() const;
    /** Get the number of parallel ranks in the simulation */
    RankInfo    getNumRanks() const;
    /** Return the base simulation Output class instance */
    Output&     getSimulationOutput() const;

    /**
       Return the simulated time since the simulation began in units specified by
       the parameter.

       @param tc TimeConverter specifying the units
    */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'getCurrentSimTime(TimeConverter tc)' "
                 "(i.e., no pointer) instead.")]]
    SimTime_t getCurrentSimTime(TimeConverter* tc) const;
    SimTime_t getCurrentSimTime(TimeConverter tc) const;

    /**
       Return the simulated time since the simulation began in the
       default timebase
    */
    inline SimTime_t getCurrentSimTime() const { return getCurrentSimTime(my_info_->defaultTimeBase); }

    /**
       Return the simulated time since the simulation began in
       timebase specified

       @param base Timebase frequency in SI Units
    */
    SimTime_t getCurrentSimTime(const std::string& base) const;

    /** Utility function to return the time since the simulation began in nanoseconds */
    SimTime_t getCurrentSimTimeNano() const;
    /** Utility function to return the time since the simulation began in microseconds */
    SimTime_t getCurrentSimTimeMicro() const;
    /** Utility function to return the time since the simulation began in milliseconds */
    SimTime_t getCurrentSimTimeMilli() const;

    /** Get the amount of real-time spent executing the run phase of
     * the simulation.
     *
     * @return real-time in seconds spent executing the run phase
     */
    double getRunPhaseElapsedRealTime() const;

    /** Get the amount of real-time spent executing the init phase of
     * the simulation.
     *
     * @return real-time in seconds spent executing the init phase
     */
    double getInitPhaseElapsedRealTime() const;

    /** Get the amount of real-time spent executing the complete phase of
     * the simulation.
     *
     * @return real-time in seconds spent executing the complete phase
     */
    double getCompletePhaseElapsedRealTime() const;


    /** Add a watch point to all handlers in the Component Tree
     */
    void addWatchPoint(WatchPoint* pt);

    /** Remove a watch point from all handlers in the Component Tree
     */
    void removeWatchPoint(WatchPoint* pt);


private:
    /** Recursively add Watch point to myself and all my children
     */
    void addWatchPointRecursive(WatchPoint* pt);

    /** Recursively removes a Watch point from myself and all my
     * children
     */
    void removeWatchPointRecursive(WatchPoint* pt);

protected:
    /** Check to see if the run mode was set to INIT
        @return true if simulation run mode is set to INIT
     */
    bool isSimulationRunModeInit() const;

    /** Check to see if the run mode was set to RUN
        @return true if simulation run mode is set to RUN
     */
    bool isSimulationRunModeRun() const;

    /** Check to see if the run mode was set to BOTH
        @return true if simulation run mode is set to BOTH
     */
    bool isSimulationRunModeBoth() const;

    /** Returns the output directory of the simulation
     *  @return Directory in which simulation outputs should be
     *  placed.  Returns empty string if output directory not set by
     *  user.
     */
    std::string& getOutputDirectory() const;

    /** Signifies that a library is required for this simulation.
     *  Causes the Factory to verify that the required library is
     *  loaded.
     *
     *  NOTE: This function should rarely be required, as most
     *  dependencies are automatically detected in the simulator core.
     *  However, if the component uses an event from another library
     *  that is not wholly defined in a header file, this call may be
     *  required to ensure that all the code from the event is loaded.
     *  Similarly, if you use a class from another library that does
     *  not have ELI information, this call may also be required to
     *  make sure all dependencies are loaded.
     *
     *  @param name Name of library this BaseComponent depends on
     */
    void requireLibrary(const std::string& name);


    /** Determine if a port name is connected to any links */
    bool isPortConnected(const std::string& name) const;

    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link.  If nullptr is passed in, then it
     * will use the Component defaultTimeBase
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */

    [[deprecated(
        "Use of shared TimeConverter objects is deprecated. Use 'configureLink(const std::string& name, TimeConverter "
        "time_base, EventHandlerBase* handler)' (i.e., no TimeConverter pointer) instead.")]]
    Link* configureLink(const std::string& name, TimeConverter* time_base, Event::HandlerBase* handler = nullptr);
    Link* configureLink(const std::string& name, TimeConverter time_base, Event::HandlerBase* handler = nullptr);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link as a string
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    Link* configureLink(const std::string& name, const std::string& time_base, Event::HandlerBase* handler = nullptr);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link as a UnitAlgebra
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    Link* configureLink(const std::string& name, const UnitAlgebra& time_base, Event::HandlerBase* handler = nullptr);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    Link* configureLink(const std::string& name, Event::HandlerBase* handler = nullptr);

    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link.  If nullptr is passed in, then it
     * will use the Component defaultTimeBase
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'configureSelfLink(const std::string& name, "
                 "TimeConverter time_base, EventHandlerBase* handler)' (i.e., no TimeConverter pointer) instead.")]]
    Link* configureSelfLink(const std::string& name, TimeConverter* time_base, Event::HandlerBase* handler = nullptr);
    Link* configureSelfLink(const std::string& name, TimeConverter time_base, Event::HandlerBase* handler = nullptr);

    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link as a string
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    Link* configureSelfLink(
        const std::string& name, const std::string& time_base, Event::HandlerBase* handler = nullptr);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link as a UnitAlgebra
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    Link* configureSelfLink(
        const std::string& name, const UnitAlgebra& time_base, Event::HandlerBase* handler = nullptr);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or nullptr if an error occured.
     */
    Link* configureSelfLink(const std::string& name, Event::HandlerBase* handler = nullptr);

    /** Registers a clock for this component.
        @param freq Frequency for the clock in SI units
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        @param regAll Should this clock period be used as the default
        time base for all of the links connected to this component
        @return the TimeConverter object representing the clock frequency
    */
    TimeConverter* registerClock(const std::string& freq, Clock::HandlerBase* handler, bool regAll = true);

    /** Registers a clock for this component.
        @param freq Frequency for the clock as a UnitAlgebra object
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        @param regAll Should this clock period be used as the default
        time base for all of the links connected to this component
        @return the TimeConverter object representing the clock frequency
    */
    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, bool regAll = true);

    /** Registers a clock for this component.
        @param tc TimeConverter object specifying the clock frequency
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        @param regAll Should this clock period be used as the default
        time base for all of the links connected to this component
        @return the TimeConverter object representing the clock frequency
    */
    [[deprecated(
        "Use of shared TimeConverter objects is deprecated. Use 'registerClock(TimeConverter tc, Clock::HandlerBase* "
        "handler, bool regAll)' (i.e., no TimeConverter pointer) instead.")]]
    TimeConverter* registerClock(TimeConverter* tc, Clock::HandlerBase* handler, bool regAll = true);
    TimeConverter* registerClock(TimeConverter tc, Clock::HandlerBase* handler, bool regAll = true);

    /** Removes a clock handler from the component */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'unregisterClock(TimeConverter tc, "
                 "Clock::HandlerBase* handler)' (i.e., no TimeConverter pointer) instead.")]]
    void unregisterClock(TimeConverter* tc, Clock::HandlerBase* handler);
    void unregisterClock(TimeConverter tc, Clock::HandlerBase* handler);

    /** Reactivates an existing Clock and Handler
     * @return time of next time clock handler will fire
     *
     * Note: If called after the simulation run loop (e.g., in finish() or complete()),
     * will return the next time of the clock past when the simulation ended. There can
     * be a small lag between simulation end and detection of simulation end during which
     * clocks can run a few extra cycles. As a result, the return value just prior to
     * simulation end may be greater than the value returned after simulation end.
     */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'reregisterClock(TimeConverter freq, "
                 "Clock::HandlerBase* handler)' (i.e., no TimeConverter pointer) instead.")]]
    Cycle_t reregisterClock(TimeConverter* freq, Clock::HandlerBase* handler);
    Cycle_t reregisterClock(TimeConverter freq, Clock::HandlerBase* handler);

    /** Returns the next Cycle that the TimeConverter would fire
        If called prior to the simulation run loop, next Cycle is 0.
        If called after the simulation run loop completes (e.g., during
        complete() or finish()), next  Cycle is one past the end time of
        the simulation. See Note in reregisterClock() for additional guidance
        when calling this function after simulation ends.
     */
    [[deprecated(
        "Use of shared TimeConverter objects is deprecated. Use 'getNextClockCycle(TimeConverter freq)' (i.e., "
        "no TimeConverter pointer) instead.")]]
    Cycle_t getNextClockCycle(TimeConverter* freq);
    Cycle_t getNextClockCycle(TimeConverter freq);

    /** Registers a default time base for the component and optionally
        sets the the component's links to that timebase. Useful for
        components which do not have a clock, but would like a default
        timebase.
        @param base Frequency for the clock in SI units
        @param regAll Should this clock period be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerTimeBase(const std::string& base, bool regAll = true);

    TimeConverter* getTimeConverter(const std::string& base) const;
    TimeConverter* getTimeConverter(const UnitAlgebra& base) const;

    bool isStatisticShared(const std::string& statName, bool include_me = false)
    {
        if ( include_me ) {
            if ( doesComponentInfoStatisticExist(statName) ) {
                return true;
            }
        }
        if ( my_info_->sharesStatistics() ) {
            return my_info_->parent_info->component->isStatisticShared(statName, true);
        }
        else {
            return false;
        }
    }

    void initiateInteractive(const std::string& msg);

private:
    ImplementSerializable(SST::BaseComponent)
    void serialize_order(SST::Core::Serialization::serializer& ser) override;


    /**
       Handles the profile points, default time base, handler tracking
       and checkpointing.
     */
    void registerClock_impl(TimeConverter* tc, Clock::HandlerBase* handler, bool regAll);

    /**
        Handles default timebase setup
    */
    Link* configureLink_impl(const std::string& name, SimTime_t time_base, Event::HandlerBase* handler = nullptr);

    template <typename T>
    Statistics::Statistic<T>* createStatistic(
        SST::Params& params, StatisticId_t id, const std::string& name, const std::string& statSubId)
    {
        /* I would prefer to avoid this std::function with dynamic cast,
         * but the code is just a lot cleaner and avoids many unnecessary template instantiations
         * doing it this way. At some point in the future, we would need to clean up
         * the rule around enabling all statistics to make this better
         */

        StatCreateFunction create = [=](BaseComponent* comp, Statistics::StatisticProcessingEngine* engine,
                                        const std::string& type, const std::string& name, const std::string& subId,
                                        SST::Params& params) -> Statistics::StatisticBase* {
            return engine->createStatistic<T>(comp, type, name, subId, params);
        };

        // We follow two distinct paths depending on if it is enable all, versus explicitly enabled
        // Enable all is "scoped" to the (sub)component
        // Explicitly enabled stats are assigned component-unique IDs and can be shared across subcomponents
        // so creation and management happens in the parent component
        Statistics::StatisticBase* base_stat =
            id == STATALL_ID ? createEnabledAllStatistic(params, name, statSubId, std::move(create))
                             : getParentComponent()->createExplicitlyEnabledStatistic(
                                   params, id, name, statSubId, std::move(create));

        // Ugh, dynamic casts hurt my eyes, but I must do this
        auto* statistic = dynamic_cast<Statistics::Statistic<T>*>(base_stat);
        if ( statistic ) {
            return statistic;
        }
        else {
            fatal(__LINE__, __FILE__, "createStatistic", 1, "failed to cast created statistic '%s' to expected type",
                name.c_str());
        }
    }

    template <typename T>
    Statistics::Statistic<T>* createNullStatistic(
        SST::Params& params, const std::string& name, const std::string& statSubId = "")
    {
        auto* engine = getStatEngine();
        return engine->createStatistic<T>(my_info_->component, "sst.NullStatistic", name, statSubId, params);
    }

    template <typename T>
    Statistics::Statistic<T>* registerStatistic(
        SST::Params& params, const std::string& statName, const std::string& statSubId, bool inserting)
    {
        if ( my_info_->enabled_stat_names_ ) {
            auto iter = my_info_->enabled_stat_names_->find(statName);
            if ( iter != my_info_->enabled_stat_names_->end() ) {
                // valid, enabled statistic
                // During initialization, the component should have assigned a mapping between
                // the local name and globally unique stat ID
                StatisticId_t id = iter->second;
                return createStatistic<T>(params, id, statName, statSubId);
            }
        }

        // if we got here, this is not a stat we explicitly enabled
        if ( inserting || doesComponentInfoStatisticExist(statName) ) {
            // this is a statistic that I registered
            if ( my_info_->enabled_all_stats_ ) {
                return createStatistic<T>(params, STATALL_ID, statName, statSubId);
            }
            else if ( my_info_->parent_info && my_info_->canInsertStatistics() ) {
                // I did not explicitly enable nor enable all
                // but I can insert statistics into my parent
                // and my parent may have enabled all
                return my_info_->parent_info->component->registerStatistic<T>(params, statName, statSubId, true);
            }
            else {
                // I did not enable, I cannot insert into parent - so send back null stat
                return my_info_->component->createNullStatistic<T>(params, statName, statSubId);
            }
        }
        else if ( my_info_->parent_info && my_info_->sharesStatistics() ) {
            // this is not a statistic that I registered
            // but my parent can share statistics, maybe they enabled
            return my_info_->parent_info->component->registerStatistic<T>(params, statName, statSubId, false);
        }
        else {
            // not a valid stat and I won't be able to share my parent's statistic
            fatal(__LINE__, __FILE__, "registerStatistic", 1, "attempting to register unknown statistic '%s'",
                statName.c_str());
        }
    }

protected:
    /** Registers a statistic.
        If Statistic is allowed to run (controlled by Python runtime parameters),
        then a statistic will be created and returned. If not allowed to run,
        then a NullStatistic will be returned.  In either case, the returned
        value should be used for all future Statistic calls.  The type of
        Statistic and the Collection Rate is set by Python runtime parameters.
        If no type is defined, then an Accumulator Statistic will be provided
        by default.  If rate set to 0 or not provided, then the statistic will
        output results only at end of sim (if output is enabled).
        @param params Parameter set to be passed to the statistic constructor.
        @param statName Primary name of the statistic.  This name must match the
               defined ElementInfoStatistic in the component, and must also
               be enabled in the Python input file.
        @param statSubId An additional sub name for the statistic
        @return Either a created statistic of desired type or a NullStatistic
                depending upon runtime settings.
    */
    template <typename T>
    Statistics::Statistic<T>* registerStatistic(
        SST::Params& params, const std::string& statName, const std::string& statSubId = "")
    {
        return registerStatistic<T>(params, statName, statSubId, false);
    }

    template <typename T>
    Statistics::Statistic<T>* registerStatistic(const std::string& statName, const std::string& statSubId = "")
    {
        SST::Params empty {};
        return registerStatistic<T>(empty, statName, statSubId, false);
    }

    template <typename... Args>
    Statistics::Statistic<std::tuple<Args...>>* registerMultiStatistic(
        const std::string& statName, const std::string& statSubId = "")
    {
        SST::Params empty {};
        return registerStatistic<std::tuple<Args...>>(empty, statName, statSubId, false);
    }

    template <typename... Args>
    Statistics::Statistic<std::tuple<Args...>>* registerMultiStatistic(
        SST::Params& params, const std::string& statName, const std::string& statSubId = "")
    {
        return registerStatistic<std::tuple<Args...>>(params, statName, statSubId, false);
    }

    template <typename T>
    Statistics::Statistic<T>* registerStatistic(const char* statName, const char* statSubId = "")
    {
        return registerStatistic<T>(std::string(statName), std::string(statSubId));
    }

    /** Called by the Components and Subcomponent to perform a statistic Output.
     * @param stat - Pointer to the statistic.
     * @param EndOfSimFlag - Indicates that the output is occurring at the end of simulation.
     */
    void performStatisticOutput(Statistics::StatisticBase* stat);

    /** Performs a global statistic Output.
     * This routine will force ALL Components and Subcomponents to output their statistic information.
     * This may lead to unexpected results if the statistic counts or data is reset on output.
     * NOTE: Currently, this function will only output statistics that are on the same rank.
     */
    void performGlobalStatisticOutput();

    /** Registers a profiling point.
        This function will register a profiling point.
        @param point Point to resgister
        @return Either a pointer to a created T::ProfilePoint or nullptr if not enabled.
    */
    template <typename T>
    typename T::ProfilePoint* registerProfilePoint(const std::string& pointName)
    {
        std::string full_point_name = getType() + "." + pointName;
        auto        tools           = getComponentProfileTools(full_point_name);
        if ( tools.size() == 0 ) return nullptr;

        typename T::ProfilePoint* ret = new typename T::ProfilePoint();
        for ( auto* x : tools ) {
            T* tool = dynamic_cast<T*>(x);
            if ( nullptr == tool ) {
                //  Not the right type, fatal
                fatal(CALL_INFO_LONG, 1, "ERROR: wrong type of profiling tool for profiling point %s)\n",
                    pointName.c_str());
            }
            ret->registerProfilePoint(tool, pointName, getId(), getName(), getType());
        }
        return ret;
    }

    /** Loads a module from an element Library
     * @param type Fully Qualified library.moduleName
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of module, or nullptr on failure.
     */
    template <class T, class... ARGS>
    T* loadModule(const std::string& type, Params& params, ARGS... args)
    {

        // Check to see if this can be loaded with new API or if we have to fallback to old
        return Factory::getFactory()->CreateWithParams<T>(type, params, params, args...);
    }

protected:
    // When you direct load, the ComponentExtension does not need any
    // ELI information and if it has any, it will be ignored.  The
    // extension will be loaded as if it were part of the parent
    // BaseComponent and will share all that components ELI
    // information.
    template <class T, class... ARGS>
    T* loadComponentExtension(ARGS... args)
    {
        ComponentExtension* ret = new T(my_info_->id_, args...);
        return static_cast<T*>(ret);
    }

    /**
       Check to see if a given element type is loadable with a particular API

       @param name - Name of element to check in lib.name format
       @return True if loadable as the API specified as the template parameter
     */
    template <class T>
    bool isSubComponentLoadableUsingAPI(const std::string& type)
    {
        return Factory::getFactory()->isSubComponentLoadableUsingAPI<T>(type);
    }

    /**
       Check to see if the element type loaded by the user into the.
       specified slot is loadable with a particular API.  This will
       only check slot index 0.  If you need to check other slots,
       please use the SubComponentSlotInfo.

       @param slot_name - Name of slot to check
       @return True if loadable as the API specified as the template parameter
     */
    template <class T>
    bool isUserSubComponentLoadableUsingAPI(const std::string& slot_name)
    {
        // Get list of ComponentInfo objects and make sure that there is
        // only one SubComponent put into this slot
        // const std::vector<ComponentInfo>& subcomps = my_info_->getSubComponents();
        const std::map<ComponentId_t, ComponentInfo>& subcomps  = my_info_->getSubComponents();
        int                                           sub_count = 0;
        int                                           index     = -1;
        for ( auto& ci : subcomps ) {
            if ( ci.second.getSlotName() == slot_name ) {
                index = ci.second.getSlotNum();
                sub_count++;
            }
        }

        if ( sub_count > 1 ) {
            SST::Output outXX("SubComponentSlotWarning: ", 0, 0, Output::STDERR);
            outXX.fatal(CALL_INFO, 1,
                "Error: ComponentSlot \"%s\" in component \"%s\" only allows for one SubComponent, %d provided.\n",
                slot_name.c_str(), my_info_->getType().c_str(), sub_count);
        }

        return isUserSubComponentLoadableUsingAPIByIndex<T>(slot_name, index);
    }

    /**
       Loads an anonymous subcomponent (not defined in input file to
       SST run).

       @param type tyupe of subcomponent to load in lib.name format
       @param slot_name name of the slot to load subcomponent into
       @param slot_num  index of the slot to load subcomponent into
       @param share_flags Share flags to be used by subcomponent
       @param params Params object to be passed to subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T, class... ARGS>
    T* loadAnonymousSubComponent(const std::string& type, const std::string& slot_name, int slot_num,
        uint64_t share_flags, Params& params, ARGS... args)
    {

        share_flags             = share_flags & ComponentInfo::USER_FLAGS;
        ComponentId_t  cid      = my_info_->addAnonymousSubComponent(my_info_, type, slot_name, slot_num, share_flags);
        ComponentInfo* sub_info = my_info_->findSubComponent(cid);

        // This shouldn't happen since we just put it in, but just in case
        if ( sub_info == nullptr ) return nullptr;

        // Check to see if this can be loaded with new API or if we have to fallback to old
        if ( isSubComponentLoadableUsingAPI<T>(type) ) {
            auto ret = Factory::getFactory()->CreateWithParams<T>(type, params, sub_info->id_, params, args...);
            return ret;
        }
        return nullptr;
    }

    /**
       Loads a user defined subcomponent (defined in input file to SST
       run).  This version does not allow share flags (set to
       SHARE_NONE) or constructor arguments.

       @param slot_name name of the slot to load subcomponent into

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T>
    T* loadUserSubComponent(const std::string& slot_name)
    {
        return loadUserSubComponent<T>(slot_name, ComponentInfo::SHARE_NONE);
    }

    /**
       Loads a user defined subcomponent (defined in input file to SST
       run).

       @param slot_name name of the slot to load subcomponent into
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T, class... ARGS>
    T* loadUserSubComponent(const std::string& slot_name, uint64_t share_flags, ARGS... args)
    {

        // Get list of ComponentInfo objects and make sure that there is
        // only one SubComponent put into this slot
        // const std::vector<ComponentInfo>& subcomps = my_info_->getSubComponents();
        const std::map<ComponentId_t, ComponentInfo>& subcomps  = my_info_->getSubComponents();
        int                                           sub_count = 0;
        int                                           index     = -1;
        for ( auto& ci : subcomps ) {
            if ( ci.second.getSlotName() == slot_name ) {
                index = ci.second.getSlotNum();
                sub_count++;
            }
        }

        if ( sub_count > 1 ) {
            SST::Output outXX("SubComponentSlotWarning: ", 0, 0, Output::STDERR);
            outXX.fatal(CALL_INFO, 1,
                "Error: ComponentSlot \"%s\" in component \"%s\" only allows for one SubComponent, %d provided.\n",
                slot_name.c_str(), my_info_->getType().c_str(), sub_count);
        }

        return loadUserSubComponentByIndex<T, ARGS...>(slot_name, index, share_flags, args...);
    }

    /** Convenience function for reporting fatal conditions.  The
        function will create a new Output object and call fatal()
        using the supplied parameters.  Before calling
        Output::fatal(), the function will also print other
        information about the (sub)component that called fatal and
        about the simulation state.

        From Output::fatal: Message will be sent to the output
        location and to stderr.  The output will be prepended with the
        expanded prefix set in the object.
        NOTE: fatal() will call MPI_Abort(exit_code) to terminate simulation.

        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param exit_code The exit code used for termination of simulation.
               will be passed to MPI_Abort()
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    [[noreturn]]
    void fatal(uint32_t line, const char* file, const char* func, int exit_code, const char* format, ...) const
        __attribute__((format(printf, 6, 7)));

    /** Convenience function for testing for and reporting fatal
        conditions.  If the condition holds, fatal() will be called,
        otherwise, the function will return.  The function will create
        a new Output object and call fatal() using the supplied
        parameters.  Before calling Output::fatal(), the function will
        also print other information about the (sub)component that
        called fatal and about the simulation state.

        From Output::fatal: Message will be sent to the output
        location and to stderr.  The output will be prepended with the
        expanded prefix set in the object.
        NOTE: fatal() will call MPI_Abort(exit_code) to terminate simulation.

        @param condition on which to call fatal(); fatal() is called
        if the bool is false.
        @param line Line number of calling function (use CALL_INFO macro)
        @param file File name calling function (use CALL_INFO macro)
        @param func Function name calling function (use CALL_INFO macro)
        @param exit_code The exit code used for termination of simulation.
               will be passed to MPI_Abort()
        @param format Format string.  All valid formats for printf are available.
        @param ... Arguments for format.
     */
    void sst_assert(bool condition, uint32_t line, const char* file, const char* func, int exit_code,
        const char* format, ...) const __attribute__((format(printf, 7, 8)));

private:
    SimTime_t processCurrentTimeWithUnderflowedBase(const std::string& base) const;

    void configureCollectionMode(Statistics::StatisticBase* statistic, const std::string& name);

    /**
     * @brief findExplicitlyEnabledStatistic
     * @param params
     * @param id
     * @param name
     * @param statSubId
     * @return that matching stat if the stat already was created for the given ID, otherwise nullptr
     */
    Statistics::StatisticBase* createExplicitlyEnabledStatistic(SST::Params& params, StatisticId_t id,
        const std::string& name, const std::string& statSubId, StatCreateFunction create);

    /**
     * @brief createStatistic Helper function used by both enable all and explicit enable
     * @param cpp_params Any parameters given in C++ specific to this statistic
     * @param python_params Any parameters given in Python for this statistic
     * @param name The name (different from type) for this statistic
     * @param statSubId An optional sub ID for this statistic if multiple stats might have the same name
     * @param create A type-erased factory for creating stats of a particulary type T
     * @return The statistic created
     */
    Statistics::StatisticBase* createStatistic(SST::Params& cpp_params, const SST::Params& python_params,
        const std::string& name, const std::string& statSubId, bool check_load_level, StatCreateFunction create);

    Statistics::StatisticBase* createEnabledAllStatistic(
        SST::Params& params, const std::string& name, const std::string& statSubId, StatCreateFunction create);

    void configureAllowedStatParams(SST::Params& params);

    void setDefaultTimeBaseForLinks(TimeConverter tc);

    void pushValidParams(Params& params, const std::string& type);

    template <class T, class... ARGS>
    T* loadUserSubComponentByIndex(const std::string& slot_name, int slot_num, int share_flags, ARGS... args)
    {

        share_flags = share_flags & ComponentInfo::USER_FLAGS;

        // Check to see if the slot exists
        ComponentInfo* sub_info = my_info_->findSubComponent(slot_name, slot_num);
        if ( sub_info == nullptr ) return nullptr;
        sub_info->share_flags = share_flags;
        sub_info->parent_info = my_info_;

        if ( isSubComponentLoadableUsingAPI<T>(sub_info->type) ) {
            auto ret = Factory::getFactory()->CreateWithParams<T>(
                sub_info->type, *sub_info->params, sub_info->id_, *sub_info->params, args...);
            return ret;
        }
        return nullptr;
    }

    template <class T>
    bool isUserSubComponentLoadableUsingAPIByIndex(const std::string& slot_name, int slot_num)
    {
        // Check to see if the slot exists
        ComponentInfo* sub_info = my_info_->findSubComponent(slot_name, slot_num);
        if ( sub_info == nullptr ) return false;

        return isSubComponentLoadableUsingAPI<T>(sub_info->type);
    }

    // Utility function used by fatal and sst_assert
    [[noreturn]]
    void vfatal(uint32_t line, const char* file, const char* func, int exit_code, const char* format, va_list arg) const
        __attribute__((format(printf, 6, 0)));

    // Get the statengine from Simulation_impl
    StatisticProcessingEngine* getStatEngine();

public:
    SubComponentSlotInfo* getSubComponentSlotInfo(const std::string& name, bool fatalOnEmptyIndex = false);

    /** Retrieve the X,Y,Z coordinates of this component */
    const std::vector<double>& getCoordinates() const { return my_info_->coordinates; }

protected:
    friend class SST::Statistics::StatisticProcessingEngine;
    friend class SST::Statistics::StatisticBase;

    bool isAnonymous() { return my_info_->isAnonymous(); }

    bool isUser() { return my_info_->isUser(); }

    /** Manually set the default defaultTimeBase */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'setDefaultTimeBase(TimeConverter tc)' "
                 "(i.e., no TimeConverter pointer) instead.")]]
    void setDefaultTimeBase(TimeConverter* tc)
    {
        my_info_->defaultTimeBase = tc;
    }

    /** Manually set the default defaultTimeBase */
    void setDefaultTimeBase(TimeConverter tc) { my_info_->defaultTimeBase = tc; }

    // Can change this back to inline once we move completely away
    // from TimeConverter*
    TimeConverter*       getDefaultTimeBase();       // { return my_info_->defaultTimeBase; }
    const TimeConverter* getDefaultTimeBase() const; // { return my_info_->defaultTimeBase; }

    bool doesSubComponentExist(const std::string& type);

    // Does the statisticName exist in the ElementInfoStatistic
    bool    doesComponentInfoStatisticExist(const std::string& statisticName) const;
    // Return the EnableLevel for the statisticName from the ElementInfoStatistic
    uint8_t getComponentInfoStatisticEnableLevel(const std::string& statisticName) const;
    // Return the Units for the statisticName from the ElementInfoStatistic
    // std::string getComponentInfoStatisticUnits(const std::string& statisticName) const;

    std::vector<Profile::ComponentProfileTool*> getComponentProfileTools(const std::string& point);

    /**** Primary Component API ****/

    /**
       Register as a primary component, which allows the component to
       specify when it is and is not OK to end simulation.  The
       simulator will not end simulation through use of the Exit
       object while any primary component has specified
       primaryComponentDoNotEndSim().  However, it is still possible
       for Actions other than Exit to end simulation.  Once all
       primary components have specified primaryComponentOKToEndSim(),
       the Exit object will trigger and end simulation.

       This must be called during simulation wireup (i.e during the
       constructor for the component), or a fatal error will occur.

       If no component registers as a primary component, then the Exit
       object will not be used for that simulation and simulation
       termination must be accomplished through some other mechanism
       (e.g. --stopAt flag, or some other Action object).

        @sa BaseComponent::primaryComponentDoNotEndSim()
        @sa BaseComponent::primaryComponentOKToEndSim()
    */
    void registerAsPrimaryComponent();

    /**
       Tells the simulation that it should not exit.  The component
       will remain in this state until a call to
       primaryComponentOKToEndSim(). A component may reenter the DoNotEndSime state
       by calling this function after calling
       primaryComponentOKToEndSim(), in which case, another call to
       primaryComponentOKToEndSim() will need to be called to end the
       simulation.

       Calls to this function when already in the DoNotEndSime state,
       will be functionally ignored, but will generate a warning if
       verbose is turned on.

       Calls to this function on non-primary components will be
       ignored and will generate a warning if verbose is turned on.

       @sa BaseComponent::registerAsPrimaryComponent()
       @sa BaseComponent::primaryComponentOKToEndSim()
    */
    void primaryComponentDoNotEndSim();

    /**
       Tells the simulation that it is now OK to end simulation.
       Simulation will not end until all primary components that have
       called primaryComponentDoNotEndSim() have called this function.

       Calls to this function when already in the OKToEndSim state,
       will be functionally ignored, but will generate a warning if
       verbose is turned on.

       Calls to this function on non-primary components will be
       ignored and will generate a warning if verbose is turned on.

       @sa BaseComponent::registerAsPrimaryComponent()
       @sa BaseComponent::primaryComponentDoNotEndSim()
    */
    void primaryComponentOKToEndSim();


private:
    enum class ComponentState : uint8_t {
        // Primary States
        None        = 0,
        Primary     = 1 << 0,
        DoNotEndSim = 1 << 1,
        OKToEndSim  = 1 << 2,

        // Whether this is an extension
        Extension = 1 << 3,
    };

    /**
       Check for Primary state

       @return true if component_state_ includes Primary, false otherwise
     */
    bool isStatePrimary()
    {
        return static_cast<uint8_t>(component_state_) & static_cast<uint8_t>(ComponentState::Primary);
    }

    /**
       Check for DoNotEndSim state. This state is mutually exclusive with OKToEndSim

       @return true if component_state_ includes DoNotEndSim, false otherwise
     */
    bool isStateDoNotEndSim()
    {
        return static_cast<uint8_t>(component_state_) & static_cast<uint8_t>(ComponentState::DoNotEndSim);
    }

    /**
       Check for OKToEndSim state. This state is mutually exclusive with DoNotEndSim

       @return true if component_state_ includes OKToEndSim
     */
    bool isStateOKToEndSim()
    {
        return static_cast<uint8_t>(component_state_) & static_cast<uint8_t>(ComponentState::OKToEndSim);
    }

    /**
       Check for Extension state

       @return true if component_state_ includes Extension, false otherwise
     */
    bool isExtension()
    {
        return static_cast<uint8_t>(component_state_) & static_cast<uint8_t>(ComponentState::Extension);
    }


    /**
       Adds Primary to the component_state_

       NOTE: This function does not check to see if the component is
       in the proper stage of simulation, that check should be done
       before calling this function.
     */
    void setStateAsPrimary()
    {
        component_state_ = static_cast<ComponentState>(
            static_cast<uint8_t>(component_state_) | static_cast<uint8_t>(ComponentState::Primary));
    }

    /**
       Adds DoNotEndSim and removes OKToEndSim to/from the
       component_state_

       NOTE: This function does not check to see if
       registerAsPrimaryComponent() has been properly called, that
       check should be done before calling this function.
     */
    void setStateDoNotEndSim()
    {
        component_state_ = static_cast<ComponentState>(
            static_cast<uint8_t>(component_state_) | static_cast<uint8_t>(ComponentState::DoNotEndSim));
        component_state_ = static_cast<ComponentState>(
            static_cast<uint8_t>(component_state_) & ~static_cast<uint8_t>(ComponentState::OKToEndSim));
    }

    /**
       Adds OKToEndSim and removes DoNotEndSim to/from the
       component_state_

       NOTE: This function does not check to see if
       registerAsPrimaryComponent() has been properly called, that
       check should be done before calling this function.
     */
    void setStateOKToEndSim()
    {
        component_state_ = static_cast<ComponentState>(
            static_cast<uint8_t>(component_state_) | static_cast<uint8_t>(ComponentState::OKToEndSim));
        component_state_ = static_cast<ComponentState>(
            static_cast<uint8_t>(component_state_) & ~static_cast<uint8_t>(ComponentState::DoNotEndSim));
    }

    /**
       Adds Extension to the component_state_
     */
    void setAsExtension()
    {
        component_state_ = static_cast<ComponentState>(
            static_cast<uint8_t>(component_state_) | static_cast<uint8_t>(ComponentState::Extension));
    }


    friend class Core::Serialization::pvt::SerializeBaseComponentHelper;


    ComponentInfo*   my_info_ = nullptr;
    Simulation_impl* sim_     = nullptr;

    // component_state_ is intialized as NotPrimary and !Extension
    ComponentState component_state_ = ComponentState::None;
    // bool             isExtension = false;

    // Need to track clock handlers for checkpointing.  We need to
    // know what clock handlers we have registered with the core
    std::vector<Clock::HandlerBase*> clock_handlers_;
    std::set<SimTime_t>              registered_clocks_;

    void  addSelfLink(const std::string& name);
    Link* getLinkFromParentSharedPort(const std::string& port, std::vector<ConfigPortModule>& port_modules);

    using StatNameMap = std::map<std::string, std::map<std::string, Statistics::StatisticBase*>>;

    std::vector<PortModule*>                            portModules;
    std::map<StatisticId_t, Statistics::StatisticBase*> m_explicitlyEnabledSharedStats;
    std::map<StatisticId_t, StatNameMap>                m_explicitlyEnabledUniqueStats;
    StatNameMap                                         m_enabled_all_stats_;

    BaseComponent* getParentComponent()
    {
        ComponentInfo* base_info = my_info_;
        while ( base_info->parent_info ) {
            base_info = base_info->parent_info;
        }
        return base_info->component;
    }
};

/**
   Used to load SubComponents when multiple SubComponents are loaded
   into a single slot (will also also work when a single SubComponent
   is loaded).
 */
class SubComponentSlotInfo
{

    BaseComponent* comp;
    std::string    slot_name;
    int            max_slot_index;

public:
    ~SubComponentSlotInfo() {}

    SubComponentSlotInfo(BaseComponent* comp, const std::string& slot_name) :
        comp(comp),
        slot_name(slot_name)
    {
        const std::map<ComponentId_t, ComponentInfo>& subcomps = comp->my_info_->getSubComponents();

        // Look for all subcomponents with the right slot name
        max_slot_index = -1;
        for ( auto& ci : subcomps ) {
            if ( ci.second.getSlotName() == slot_name ) {
                if ( ci.second.getSlotNum() > static_cast<int>(max_slot_index) ) {
                    max_slot_index = ci.second.getSlotNum();
                }
            }
        }
    }

    const std::string& getSlotName() const { return slot_name; };

    bool isPopulated(int slot_num) const
    {
        if ( slot_num > max_slot_index ) return false;
        if ( comp->my_info_->findSubComponent(slot_name, slot_num) == nullptr ) return false;
        return true;
    }

    bool isAllPopulated() const
    {
        for ( int i = 0; i < max_slot_index; ++i ) {
            if ( comp->my_info_->findSubComponent(slot_name, i) == nullptr ) return false;
        }
        return true;
    }

    int getMaxPopulatedSlotNumber() const { return max_slot_index; }

    /**
       Check to see if the element type loaded by the user into the
       specified slot index is loadable with a particular API.

       @param slot_num Slot index to check
       @return True if loadable as the API specified as the template parameter
     */
    template <class T>
    bool isLoadableUsingAPI(int slot_num)
    {
        return comp->isUserSubComponentLoadableUsingAPIByIndex<T>(slot_name, slot_num);
    }

    // Create functions that support the new API

    /**
       Create a user defined subcomponent (defined in input file to
       SST run).  This call will pass SHARE_NONE to the new
       subcomponent and will not take constructor arguments.  If
       constructor arguments are needed for the API that is being
       loaded, the full call to create will need to be used
       create(slot_num, share_flags, args...).

       @param slot_num Slot index from which to load subcomponent

       This function supports the new API, but is identical to an
       existing API call.  It will try to load using new API and will
       fallback to old if unsuccessful.
    */
    template <typename T>
    T* create(int slot_num) const
    {
        Params empty;
        return comp->loadUserSubComponentByIndex<T>(slot_name, slot_num, ComponentInfo::SHARE_NONE);
        // return private_create<T>(slot_num, empty);
    }

    /**
       Create a user defined subcomponent (defined in input file to SST
       run).

       @param slot_num Slot index from which to load subcomponent
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T, class... ARGS>
    T* create(int slot_num, uint64_t share_flags, ARGS... args) const
    {
        return comp->loadUserSubComponentByIndex<T, ARGS...>(slot_name, slot_num, share_flags, args...);
    }

    /**
       Create all user defined subcomponents (defined in input file to SST
       run) for the slot.

       @param vec Vector of T* that will hold the pointers to the new
       subcomponents.  If an index is not occupied, a nullptr will be
       put in it's place.  All components will be added to the end of
       the vector, so index N will be at vec.length() + N, where
       vec.length() is the length of the vector when it is passed to
       the call.
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <typename T, class... ARGS>
    void createAll(std::vector<T*>& vec, uint64_t share_flags, ARGS... args) const
    {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, share_flags, args...);
            vec.push_back(sub);
        }
    }

    /**
       Create all user defined subcomponents (defined in input file to SST
       run) for the slot.

       @param vec Vector of pair<int,T*> that will hold the pointers
       to the new subcomponents.  The int will hold the index from
       which the subcomponent wass loaded.  Unoccupied indexes will be
       skipped.  All components will be added to the end of the
       vector.
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <typename T, class... ARGS>
    void createAllSparse(std::vector<std::pair<int, T*>>& vec, uint64_t share_flags, ARGS... args) const
    {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, share_flags, args...);
            if ( sub != nullptr ) vec.push_back(i, sub);
        }
    }

    /**
       Create all user defined subcomponents (defined in input file to SST
       run) for the slot.

       @param vec Vector of T* that will hold the pointers
       to the new subcomponents.  Unoccupied indexes will be
       skipped.  All components will be added to the end of the
       vector.
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <typename T, class... ARGS>
    void createAllSparse(std::vector<T*>& vec, uint64_t share_flags, ARGS... args) const
    {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, share_flags, args...);
            if ( sub != nullptr ) vec.push_back(sub);
        }
    }
};

namespace Core::Serialization {

namespace pvt {
class SerializeBaseComponentHelper
{
public:
    static void size_basecomponent(serializable_base* s, serializer& ser);

    static void pack_basecomponent(serializable_base* s, serializer& ser);

    static void unpack_basecomponent(serializable_base*& s, serializer& ser);

    static void map_basecomponent(serializable_base*& s, serializer& ser, const std::string& name);
};

} // namespace pvt


template <class T>
class serialize_impl<T*, std::enable_if_t<std::is_base_of_v<SST::BaseComponent, T>>>
{
    void operator()(T*& s, serializer& ser, ser_opt_t UNUSED(options))
    {
        serializable_base* sp = static_cast<serializable_base*>(s);
        switch ( ser.mode() ) {
        case serializer::SIZER:
            pvt::SerializeBaseComponentHelper::size_basecomponent(sp, ser);
            break;
        case serializer::PACK:
            pvt::SerializeBaseComponentHelper::pack_basecomponent(sp, ser);
            break;
        case serializer::UNPACK:
            pvt::SerializeBaseComponentHelper::unpack_basecomponent(sp, ser);
            break;
        case serializer::MAP:
            pvt::SerializeBaseComponentHelper::map_basecomponent(sp, ser, ser.getMapName());
            break;
        }
        s = static_cast<T*>(sp);
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace Core::Serialization

} // namespace SST

#endif // SST_CORE_BASECOMPONENT_H
