// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_SUBCOMPONENT_H
#define SST_CORE_SUBCOMPONENT_H

#include <sst/core/component.h>

namespace SST {


/**
   SubComponent is a class loadable through the factory which allows
   dynamic functionality to be added to a Component.  The
   SubComponent API is nearly identical to the Component API and all
   the calls are proxied to the parent Compoent.
*/
class SubComponent : public Module {

public:
	SubComponent(Component* parent);
	virtual ~SubComponent();

    /** Used during the init phase.  The method will be called each phase of initialization.
     Initialization ends when no components have sent any data. */
    virtual void init(unsigned int phase) {}
    /** Called after all components have been constructed and inialization has
	completed, but before simulation time has begun. */
    virtual void setup( ) { }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual void finish( ) { }

protected:
    /* Component* const parent; */
    Component* parent;

    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, Event::HandlerBase* handler = NULL);

    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, Event::HandlerBase* handler = NULL);

    bool doesSubComponentInfoStatisticExist(std::string statisticName);

    template <typename T>
    Statistic<T>* registerStatistic(std::string statName, std::string statSubId = "")
    {
        // Verify here that name of the stat is one of the registered
        // names of the component's ElementInfoStatisticEnable.  
        if (false == doesSubComponentInfoStatisticExist(statName)) {
            printf("Error: Statistic %s name %s is not found in ElementInfoStatisticEnable, exiting...\n",
                   StatisticBase::buildStatisticFullName(parent->getName().c_str(), statName, statSubId).
                   c_str(),
                   statName.c_str());
            exit(1);
        }
        return parent->registerStatisticCore<T>(statName, statSubId);
    }

    /** Registers a clock for this component.
        @param freq Frequency for the clock in SI units
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        NOTE:  Unlike Components, SubComponents do not have a default timebase
    */
    TimeConverter* registerClock( std::string freq, Clock::HandlerBase* handler);
    TimeConverter* registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler);

    /** Removes a clock handler from the component */
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);

    /** Reactivates an existing Clock and Handler
     * @return time of next time clock handler will fire
     */
    Cycle_t reregisterClock(TimeConverter *freq, Clock::HandlerBase* handler);
    /** Returns the next Cycle that the TimeConverter would fire */
    Cycle_t getNextClockCycle(TimeConverter *freq);
    
    // For now, no OneShot support in SubComponent
#if 0
    /** Registers a OneShot event for this component.
        Note: OneShot cannot be canceled, and will always callback after
          the timedelay.  
        @param timeDelay Time delay for the OneShot in SI units
        @param handler Pointer to OneShot::HandlerBase which is to be invoked
        at the specified interval
    */
    TimeConverter* registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler);
    TimeConverter* registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler);
#endif

    TimeConverter* getTimeConverter( const std::string& base );
    TimeConverter* getTimeConverter( const UnitAlgebra& base );

    /** return the time since the simulation began in units specified by
        the parameter.
        @param tc TimeConverter specificing the units */
    SimTime_t getCurrentSimTime(TimeConverter *tc) const;

    /** return the time since the simulation began in timebase specified
        @param base Timebase frequency in SI Units */
    SimTime_t getCurrentSimTime(std::string base);

    /** Utility function to return the time since the simulation began in nanoseconds */ 
    SimTime_t getCurrentSimTimeNano() const;
    /** Utility function to return the time since the simulation began in microseconds */ 
    SimTime_t getCurrentSimTimeMicro() const;
    /** Utility function to return the time since the simulation began in milliseconds */ 
    SimTime_t getCurrentSimTimeMilli() const;

private:
    /** Component's type, set by the factory when the object is created.
        It is identical to the configuration string used to create the
        component. I.e. the XML "<component id="aFoo"><foo>..." would
        set component::type to "foo" */
    std::string type;
    SubComponent();
//      friend class boost::serialization::access;
//      template<class Archive>
//      void
//      serialize(Archive & ar, const unsigned int version )
//      {
//      }
    };
} //namespace SST

// BOOST_CLASS_EXPORT_KEY(SST::SubComponent)

#endif // SST_CORE_SUBCOMPONENT_H
