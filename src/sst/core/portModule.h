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

#ifndef SST_CORE_PORTMODULE_H
#define SST_CORE_PORTMODULE_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/event.h"
#include "sst/core/link.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statengine.h"

using namespace SST::Statistics;

namespace SST {

class BaseComponent;

/**
   PortModules are modules that can be attached to the send and/or
   receive side of ports. Each PortModule is attached to one port and
   uses the Event::HandlerBase::InterceptPoint for intercepting
   incoming events and the Link::AttachPoint to intercept outgoing
   events.  The intercepted events can be inspected, modified and/or
   canceled. For canceled events, the PortModule is required to delete
   the event.

   NOTE: Not Final API - PortModules will continue to be supported in
   the future, but the API will not be finalized until the SST 15
   release, so there may be slight changes to the base class.

   NOTE: Attaching to a port on the send-side has known performance
   issues, so it is recommended to attach to the input port whenever
   possible.

 */
class PortModule :
    public Event::HandlerBase::InterceptPoint,
    public Link::AttachPoint,
    public SST::Core::Serialization::serializable
{

public:
    SST_ELI_DECLARE_BASE(PortModule)
    // declare extern to limit compile times
    SST_ELI_DECLARE_CTOR_EXTERN(SST::Params&)
    // These categories will print in sst-info in the order they are
    // listed here
    SST_ELI_DECLARE_INFO_EXTERN(
        ELI::ProvidesParams,
        ELI::ProvidesStats,
        ELI::ProvidesAttributes)

    PortModule();
    virtual ~PortModule() {};

    /******* Functions inherited from Link::AttachPoint *******/
    /**
       Function that will be called when a PortModule is registered on
       sends (i.e. installOnSend() returns true). The uintptr_t
       returned from this function will be passed into the eventSent()
       function.

       The default implemenation will just return 0 and only needs to
       be overwritten if the module needs any of the metadata and/or
       needs to return a unique key.

       @param mdata Metadata to be passed into the tool.  NOTE: The
       metadata will be of type EventHandlerMetaData (see
       sst/core/event.h)

       @return Opaque key that will be passed back into eventSent() to
       identify the source of the call
    */
    virtual uintptr_t registerLinkAttachTool(const AttachPointMetaData& mdata) override;

    /**
       Function that will be called when an event is sent on a
       link with registered PortModules.  If ev is set to
       nullptr, then the event will not be delivered and the function
       should delete the original event.

       NOTE: It is possible to delete the incoming event and replace
       it with a new event, if this is done, the new event MUST call
       copyAllDeliveryInfo(ev) or the event will not be properly
       processed.

       @param key Opaque key returned from registerLinkAttachTool()

       @param ev Event to intercept
    */
    virtual void eventSent(uintptr_t key, Event*& ev) override = 0;


    /**
       Function that will be called to handle the key returned from
       registerLinkAttachTool, if the AttachPoint tool is
       serializable.  This is needed because the key is opaque to the
       Link, so it doesn't know how to handle it during serialization.
       During SIZE and PACK phases of serialization, the tool needs to
       store out any information that will be needed to recreate data
       that is reliant on the key.  On UNPACK, the function needs to
       recreate the any state and reinitialize the passed in key
       reference to the proper state to continue to make valid calls
       to eventSent().

       The default implemenation will just set key to 0 on UNPACK and
       only needs to be overwritten if the module needs to return a
       unique key from registerLinkAttachTool().

       @param ser Serializer to use for serialization

       @param key Key that would be passed into the eventSent() function.
    */
    virtual void serializeEventAttachPointKey(SST::Core::Serialization::serializer& ser, uintptr_t& key) override;


    /******* Functions inherited from Event::HandlerBase::InterceptPoint *******/
    /**
       Function that will be called when a handler is registered with
       recieves (i.e. installOnReceive() returns true). The uintptr_t
       returned from this function will be passed into the intercept()
       function.

       The default implemenation will just return 0 and only needs to
       be overwritten if the module needs any of the metadata and/or
       needs to return a unique key.

       @param mdata Metadata to be passed into the tool.  NOTE: The
       metadata will be of type EventHandlerMetaData (see
       sst/core/event.h)

       @return Opaque key that will be passed back into the
       interceptHandler() calls
    */
    virtual uintptr_t registerHandlerIntercept(const AttachPointMetaData& mdata) override;


    /**
       Function that will be called before the event handler to let
       the attach point intercept the data.  The data can be modified,
       and if cancel is set to true, the handler will not be executed.
       If cancel is set to true, then the function should also delete
       the event and set data to nullptr.

       NOTE: It is possible to delete the incoming event and replace
       the it with a new event, if this is done, the new event MUST
       call copyAllDeliveryInfo(ev) or the event will not be properly
       processed.

       @param key Key returned from registerHandlerIntercept()
       function

       @param data Event that is to be passed to the handler

       @param[out] cancel Set to true if the handler delivery should
       be cancelled. If set to true, function must also delete the
       event
    */
    virtual void interceptHandler(uintptr_t key, Event*& data, bool& cancel) override = 0;

    /**
       Function that will be called to handle the key returned from
       registerHandlerIntercept, if the AttachPoint tool is
       serializable.  This is needed because the key is opaque to the
       Link, so it doesn't know how to handle it during serialization.
       During SIZE and PACK phases of serialization, the tool needs to
       store out any information that will be needed to recreate data
       that is reliant on the key.  On UNPACK, the function needs to
       recreate any state and reinitialize the passed in key reference
       to the proper state to continue to make valid calls to
       interceptHandler().

       The default implemenation will just set key to 0 on UNPACK and
       only needs to be overwritten if the module needs to return a
       unique key from registerHandlerIntercept().

       @param ser Serializer to use for serialization

       @param key Key that would be passed into the interceptHandler() function.
    */
    virtual void serializeHandlerInterceptPointKey(SST::Core::Serialization::serializer& ser, uintptr_t& key) override;

    /*** Functions that control whether module is install on send and/or receive ***/
    /**
       Called to determine if the PortModule should be installed on
       receives

       @return true if PortModule should be installed on receives
     */
    virtual bool installOnReceive() { return false; }

    /**
       Called to determine if the PortModule should be installed on
       sends. NOTE: Installing PortModules on sends will have a
       noticeable impact on performance, consider architecting things
       so that you can intercept on receives.

       @return true if PortModule should be installed on sends
     */
    virtual bool installOnSend() { return false; }

    const std::string& getName() const;

protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override;

    /*** Core API functions available to PortModules ***/

    /** Get the core timebase */
    UnitAlgebra getCoreTimeBase() const;

    /** Return the current simulation time as a cycle count*/
    SimTime_t getCurrentSimCycle() const;

    /** Return the current priority */
    int getCurrentPriority() const;

    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;

    /** Return the base simulation Output class instance */
    Output& getSimulationOutput() const;

    /**
       Return the simulated time since the simulation began in units
       specified by the parameter.

       @param tc TimeConverter specifying the units
    */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'getCurrentSimTime(TimeConverter& timebase)' "
                 "(i.e., no pointer) instead.")]]
    SimTime_t getCurrentSimTime(TimeConverter* tc) const;
    SimTime_t getCurrentSimTime(TimeConverter& tc) const;

    /**
       Return the simulated time since the simulation began in
       timebase specified

       @param base Timebase frequency in SI Units
    */
    SimTime_t getCurrentSimTime(const std::string& base) const;

    /**
       Utility function to return the time since the simulation began
       in nanoseconds
    */
    SimTime_t getCurrentSimTimeNano() const;

    /**
       Utility function to return the time since the simulation began
       in microseconds
    */
    SimTime_t getCurrentSimTimeMicro() const;

    /**
       Utility function to return the time since the simulation began
       in milliseconds
    */
    SimTime_t getCurrentSimTimeMilli() const;

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

    /** Registers a statistic.
         If Statistic is allowed to exist (controlled by Python runtime parameters),
         then a statistic will be created and returned. If not allowed to exist,
         then a NullStatistic will be returned.  The type of Statistic is set by Python
         runtime parameters.
         @param params Additional parameter set to be passed to the statistic constructor.
         @param stat_name Primary name of the statistic. This name must match the
                defined ElementInfoStatistic in the component, and must also
                be enabled in the Python input file.
         @param stat_sub_id An additional sub name for the statistic
         @return Either a created statistic or a NullStatistic depending upon runtime settings.
     */
    template <typename T>
    Statistics::Statistic<T>* registerStatistic(
        SST::Params& params, const std::string& stat_name, const std::string& stat_sub_id = "")
    {
        return registerStatistic_impl<T>(params, stat_name, stat_sub_id);
    }

    template <typename T>
    Statistics::Statistic<T>* registerStatistic(const std::string& stat_name, const std::string& stat_sub_id = "")
    {
        SST::Params empty {};
        return registerStatistic_impl<T>(empty, stat_name, stat_sub_id);
    }

    virtual std::string getEliType() const { return ""; }

private:
    friend class BaseComponent;

    /**
       Component that owns this PortModule
     */
    BaseComponent* component_ = nullptr;

    PortModuleId_t id_; // Together with component_, uniquely identifies a port module

    std::string name_;

    // Utility function used by fatal and sst_assert
    [[noreturn]]
    void vfatal(uint32_t line, const char* file, const char* func, int exit_code, const char* format, va_list arg) const
        __attribute__((format(printf, 6, 0)));

    /**
       Internal function to manage statistic registration
     */
    template <typename T>
    Statistics::Statistic<T>* registerStatistic_impl(
        SST::Params& params, const std::string& stat_name, const std::string& stat_sub_id)
    {
        // Lookup statistic enable level in ELI; if not present, returns 255
        uint8_t level = getStatisticValidityAndLevel(stat_name);
        if ( level == 255 ) {
            fatal(__LINE__, __FILE__, "registerStatistic", 1,
                "attempting to register a statistic '%s' that is not found in ELI", stat_name.c_str());
        }

        Params cfg_params;
        bool   enabled;
        std::tie(enabled, cfg_params) = isStatisticEnabled(stat_name, level);

        if ( enabled ) {
            cfg_params.insert(params);
            auto stat = getStatEngine()->createStatistic<T>(component_, stat_name, stat_sub_id, cfg_params);
            stat->setPortModName(id_.first, id_.second);
            return stat;
        }
        else {
            return getStatEngine()->createDisabledStatistic<T>();
        }
    }

    /** Helper functions for statistic enablement */
    /**
       Returns the local stat engine instance
     */
    Statistics::StatisticProcessingEngine* getStatEngine() const;

    /**
       Returns the required statistic level if stat exists in ELI, else returns 255
     */
    uint8_t getStatisticValidityAndLevel(const std::string& statistic_name) const;

    /**
       Returns the name of the
     */
    std::pair<bool, Params> isStatisticEnabled(const std::string& statistic_name, const uint8_t min_level);
};

} // namespace SST

// Macro used to register PortModules in the ELI database
#define SST_ELI_REGISTER_PORTMODULE(cls, lib, name, version, desc) \
    SST_ELI_REGISTER_DERIVED(SST::PortModule,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)                                       \
    std::string getEliType() const override                        \
    {                                                              \
        std::string ret(lib);                                      \
        return ret + "." + name;                                   \
    }

#endif // SST_CORE_PORTMODULE_H
