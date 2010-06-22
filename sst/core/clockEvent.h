// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CLOCKEVENT_H
#define SST_CLOCKEVENT_H

#include <deque>

#include <sst/core/action.h>
#include <sst/core/clockHandler.h>

#define _CLE_DBG( fmt, args...)__DBG( DBG_CLOCKEVENT, ClockEvent, fmt, ## args )

namespace SST {

class TimeConverter;

class ClockEvent : public Action
{
public:

    typedef enum { DEFAULT, PRE, POST } Which_t;

    ClockEvent( TimeConverter* period );

    bool HandlerRegister( Which_t which, ClockHandler_t* handler ); 
    bool HandlerUnregister( Which_t which, ClockHandler_t* handler, 
                                                            bool& empty ); 
private:

    typedef std::deque<ClockHandler_t*> HandlerMap_t;

    ClockEvent() { }

    void execute( void );

    Cycle_t         currentCycle;
    TimeConverter*  period;
    HandlerMap_t    handlerMap[3];

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & boost::serialization::base_object<Action>(*this);
        ar & BOOST_SERIALIZATION_NVP( currentCycle );
        ar & BOOST_SERIALIZATION_NVP( period );
        ar & BOOST_SERIALIZATION_NVP( handlerMap );
    }
};

} // namespace SST

#endif // SST_CLOCKEVENT_H
