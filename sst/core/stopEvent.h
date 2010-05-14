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


#ifndef _SST_STOPEVENT_H
#define _SST_STOPEVENT_H

#include <sst/event.h>

namespace SST {

#define _STOP_DBG( fmt, args...) __DBG( DBG_STOP, StopEvent, fmt, ## args )

class StopEvent : public Event
{
    public:
        StopEvent()
        {
	  functor = new EventHandler< StopEvent, bool, Event* >( this, &StopEvent::handler );
        }

/*         StopEvent( const StopEvent& e ) : */
/*             Event(), */
/*             functor(  new EventHandler< StopEvent, bool, Event* > */
/*                                         ( this, &StopEvent::handler ) ) */
/*         { */
/*             SetHandler( functor ); */
/*         } */

	EventHandler< StopEvent, bool, Event* >* getFunctor() {
	  return functor;
	}
	
    private:
        EventHandler< StopEvent, bool, Event* >* functor;

        bool handler( Event* e ) {
             _STOP_DBG("\n");
            return true;
        }

#if WANT_CHECKPOINT_SUPPORT

        BOOST_SERIALIZE {
            BOOST_VOID_CAST_REGISTER( StopEvent*, Event* );
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event );
        }

#endif

};

} // namespace SST

#endif
