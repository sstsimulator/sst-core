// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_CLOCK_H
#define SST_CORE_CLOCK_H

//#include <deque>
//#include <list>
#include <vector>
#include <cinttypes>

#include <sst/core/action.h>
//#include <sst/core/clockHandler.h>

#define _CLE_DBG( fmt, args...)__DBG( DBG_CLOCK, Clock, fmt, ## args )

namespace SST {

class TimeConverter;

/**
 * A Clock class.
 *
 * Calls callback functions (handlers) on a specified period
 */
class Clock : public Action
{
public:

    /** Create a new clock with a specified period */
    Clock( TimeConverter* period, int priority = CLOCKPRIORITY);
    ~Clock();

    /** Functor classes for Clock handling */
    class HandlerBase {
    public:
        /** Function called when Handler is invoked */
        virtual bool operator()(Cycle_t) = 0;
        virtual ~HandlerBase() {}
    };


    /** Event Handler class with user-data argument
     * @tparam classT Type of the Object
     * @tparam argT Type of the argument
     */
    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
        typedef bool (classT::*PtrMember)(Cycle_t, argT);
        classT* object;
        const PtrMember member;
        argT data;
	
    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         * @param data - Additional argument to pass to handler
         */
        Handler( classT* const object, PtrMember member, argT data ) :
            object(object),
            member(member),
            data(data)
        {}

        bool operator()(Cycle_t cycle) override {
            return (object->*member)(cycle,data);
        }
    };

    /** Event Handler class without user-data
     * @tparam classT Type of the Object
     */
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
        typedef bool (classT::*PtrMember)(Cycle_t);
        classT* object;
        const PtrMember member;

    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         */
        Handler( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()(Cycle_t cycle) override {
            return (object->*member)(cycle);
        }
    };

    /**
     * Activates this clock object, by inserting into the simulation's
     * timeVortex for future execution.
     */
    void schedule();

    /** Return the time of the next clock tick */
    Cycle_t getNextCycle();

    /** Add a handler to be called on this clock's tick */
    bool registerHandler( Clock::HandlerBase* handler );
    /** Remove a handler from the list of handlers to be called on the clock tick */
    bool unregisterHandler( Clock::HandlerBase* handler, bool& empty );

    void print(const std::string& header, Output &out) const override;
    
private:
/*     typedef std::list<Clock::HandlerBase*> HandlerMap_t; */
    typedef std::vector<Clock::HandlerBase*> StaticHandlerMap_t;


    Clock() { }

    void execute( void ) override;

    Cycle_t            currentCycle;
    TimeConverter*     period;
    StaticHandlerMap_t staticHandlerMap;
    SimTime_t          next;
    bool               scheduled;
    
};

} // namespace SST


#endif // SST_CORE_CLOCK_H
