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


#ifndef _SST_SYNCEVENT_H
#define _SST_SYNCEVENT_H

#include <sst/core/event.h>

namespace SST {

class SyncEvent : public Event
{
public:
     SyncEvent( Handler_t* handler )
     {
       functor = handler;
     }

     SyncEvent() {}
 SyncEvent( const SyncEvent& e ) :
            Event( e ) 
        {}

private:
	    Handler_t* functor;
	    

#if WANT_CHECKPOINT_SUPPORT

	BOOST_SERIALIZE {
            _AR_DBG( SyncEvent, "\n" );
            BOOST_VOID_CAST_REGISTER( SyncEvent*, Event* );
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event );
            _AR_DBG( SyncEvent, "\n" );
        }

        SAVE_CONSTRUCT_DATA( SyncEvent )
        {
            _AR_DBG( SyncEvent, "\n" );

            Handler_t* handler = t->functor;
            ar << BOOST_SERIALIZATION_NVP( handler );
        }

        LOAD_CONSTRUCT_DATA( SyncEvent )
        {
            _AR_DBG( SyncEvent, "\n" );

            Handler_t* handler;
            _AR_DBG( SyncEvent, "\n" );
            ar >> BOOST_SERIALIZATION_NVP( handler );
            _AR_DBG( SyncEvent, "\n" );

            ::new(t)SyncEvent( handler );
        }

#endif

};

} // namespace SST

#endif
