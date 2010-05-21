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

#include <sst/core/event.h>
#include <sst/core/clockHandler.h>

#define _CLE_DBG( fmt, args...)__DBG( DBG_CLOCKEVENT, ClockEvent, fmt, ## args )

namespace SST {

class TimeConverter;

class ClockEvent : public Event
{
    public:

        typedef enum { DEFAULT, PRE, POST } Which_t;
        ClockEvent( TimeConverter* period );

        bool HandlerRegister( Which_t which, ClockHandler_t* handler ); 
        bool HandlerUnregister( Which_t which, ClockHandler_t* handler, 
                                                            bool& empty ); 

	EventHandler< ClockEvent, bool, Event* >* getFunctor() {
	  return functor;
	}
	
    private:

        typedef std::deque<ClockHandler_t*> HandlerMap_t;

        EventHandler< ClockEvent, bool, Event* >* functor;

        Cycle_t         currentCycle;
	TimeConverter*  period;
        HandlerMap_t    handlerMap[3];

        bool handler( Event* e );

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<Event>(*this);
        ar & BOOST_SERIALIZATION_NVP( currentCycle );
        ar & BOOST_SERIALIZATION_NVP( period );
        ar & BOOST_SERIALIZATION_NVP( handlerMap );
        ar & BOOST_SERIALIZATION_NVP( functor );
    }

    template<class Archive>
    friend void 
    save_construct_data(Archive & ar, 
                        const ClockEvent * t,
                        const unsigned int file_version)
    {
        TimeConverter*  period = t->period; 
        ar << BOOST_SERIALIZATION_NVP( period );
    }

    template<class Archive>
    friend void 
    load_construct_data(Archive & ar, 
                        ClockEvent * t, 
                        const unsigned int file_version)
    {
        TimeConverter*  period;
        ar >> BOOST_SERIALIZATION_NVP( period );
        ::new(t)ClockEvent( period );
    }
};

} // namespace SST

#endif // SST_CLOCKEVENT_H
