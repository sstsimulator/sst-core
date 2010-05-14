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

#if WANT_CHECKPOINT_SUPPORT
	
        BOOST_SERIALIZE {
            _AR_DBG( ClockEvent, "Starting\n" );
            BOOST_VOID_CAST_REGISTER( ClockEvent*, Event* );
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event );
            _AR_DBG( ClockEvent, "\n" );
            ar & BOOST_SERIALIZATION_NVP( currentCycle );
            _AR_DBG( ClockEvent, "\n" );
            ar & BOOST_SERIALIZATION_NVP( period );
            _AR_DBG( ClockEvent, "\n" );
            ar & BOOST_SERIALIZATION_NVP( handlerMap );
            _AR_DBG( ClockEvent, "\n" );
            ar & BOOST_SERIALIZATION_NVP( functor );
            _AR_DBG( ClockEvent, "\n" );

            _AR_DBG( ClockEvent, "Done\n" );
        }

        SAVE_CONSTRUCT_DATA( ClockEvent )
        {
            _AR_DBG( ClockEvent, "Save construct data\n" );

            TimeConverter*  period = t->period; 

            ar << BOOST_SERIALIZATION_NVP( period );
        }

        LOAD_CONSTRUCT_DATA( ClockEvent )
        {
            _AR_DBG( ClockEvent, "Load construct data\n" );

            TimeConverter*  period;

            ar >> BOOST_SERIALIZATION_NVP( period );

            ::new(t)ClockEvent( period );
        }

#endif
	
};

} // namespace SST

#endif // SST_CLOCKEVENT_H
