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


#ifndef SST_CLOCK_H
#define SST_CLOCK_H

#include <deque>

#include <sst/core/action.h>
#include <sst/core/clockHandler.h>

#define _CLE_DBG( fmt, args...)__DBG( DBG_CLOCK, Clock, fmt, ## args )

namespace SST {

class TimeConverter;

class Clock : public Action
{
public:
    typedef enum { DEFAULT, PRE, POST } Which_t;

    Clock( TimeConverter* period );

    bool HandlerRegister( Which_t which, ClockHandler_t* handler ); 
    bool HandlerUnregister( Which_t which, ClockHandler_t* handler, 
                                                            bool& empty ); 
private:
    typedef std::deque<ClockHandler_t*> HandlerMap_t;

    Clock() { }

    void execute( void );

    Cycle_t         currentCycle;
    TimeConverter*  period;
    HandlerMap_t    handlerMap[3];

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
        ar & BOOST_SERIALIZATION_NVP(currentCycle);
        ar & BOOST_SERIALIZATION_NVP(period);
    }
};

} // namespace SST

#endif // SST_CLOCK_H
