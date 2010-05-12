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

#include <sst_config.h>

#include "dummy.h"
#include <sst/memEvent.h>

// bool Dummy::clock( Cycle_t current, Time_t epoch ) {
bool Dummy::clock( Cycle_t current ) {

    MemEvent *event;
    if ( ( event = static_cast< MemEvent* >( cpu->Recv() ) ) )
    {
        _DUMMY_DBG("got a event from the cpu address %#lx\n", event->address );
        nic->Send( event );
    }
    return false;
}

// bool Dummy::processEvent( Time_t time, Event* event  )
bool Dummy::processEvent( Event* event  )
{
//     _DUMMY_DBG( "id=%lu time=%.9f\n", Id(), time );
    _DUMMY_DBG( "id=%lu time=FIXME\n", Id() );
    cpu->Send( static_cast<CompEvent*>(event) );
    return false;
}

#if WANT_CHECKPOINT_SUPPORT

BOOST_CLASS_EXPORT(Dummy)
// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Dummy, bool, SST::Cycle_t, SST::Time_t )

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Dummy, bool, SST::Time_t, SST::Event* )

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Dummy, bool, SST::Cycle_t )

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Dummy, bool, SST::Event* )

#endif
