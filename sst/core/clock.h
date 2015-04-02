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


#ifndef SST_CORE_CLOCK_H
#define SST_CORE_CLOCK_H

#include <sst/core/serialization.h>

#include <vector>

#include <sst/core/action.h>
#include <sst/core/handler.h>

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

    // Functor classes for Clock handling (derived from handler.h)
    using HandlerBase = SST::HandlerBase<Cycle_t, bool>;
    template<typename classT, typename argT = void> using Handler = SST::Handler<Cycle_t, bool, classT, argT>;


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

    void print(const std::string& header, Output &out) const;
    
private:
/*     typedef std::list<Clock::HandlerBase*> HandlerMap_t; */
    typedef std::vector<Clock::HandlerBase*> StaticHandlerMap_t;


    Clock() { }

    void execute( void );

    Cycle_t            currentCycle;
    TimeConverter*     period;
    StaticHandlerMap_t staticHandlerMap;
    SimTime_t          next;
    bool               scheduled;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
        ar & BOOST_SERIALIZATION_NVP(currentCycle);
        ar & BOOST_SERIALIZATION_NVP(period);
        ar & BOOST_SERIALIZATION_NVP(staticHandlerMap);
        ar & BOOST_SERIALIZATION_NVP(scheduled);
    }
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Clock)

#endif // SST_CORE_CLOCK_H
