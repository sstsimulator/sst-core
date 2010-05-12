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

#include "xbar.h"
// #include <sst/memEvent.h>
#include "../cpu/myMemEvent.h"

BOOST_CLASS_EXPORT( SST::MyMemEvent )
//BOOST_IS_MPI_DATATYPE( SST::MyMemEvent )

// bool Xbar::clock( Cycle_t current, Time_t epoch ) {
bool Xbar::clock( Cycle_t current ) {
    //_XBAR_DBG( "id=%lu currentCycle=%lu\n", Id(), current );

  // Need to initalize the self event for pushes on clock cycle 1
  if ( current == 1 ) {
    _XBAR_DBG("initializing self link.\n" );
    selfPush->Send(0,new MyMemEvent());
  }
  
  // Need to initalize the self event for pulls on clock cycle 6
  if ( current == 6 ) {
    _XBAR_DBG("initializing self link.\n" );
    selfPull->Send(0,new MyMemEvent());
  }
  
    MyMemEvent *event;
    if ( ( event = static_cast< MyMemEvent* >( selfPull->Recv() ) ) )
    {
      _XBAR_DBG("got a pulled self event @ cycle %ld\n", current );
      selfPull->Send(50,event);
    }
    if ( ( event = static_cast< MyMemEvent* >( cpu->Recv() ) ) )
    {
        _XBAR_DBG("got an event from the cpu address %#lx @ cycle %ld\n", event->address, current );
        nic->Send( 30, event );
    }
    return false;
}

// bool Xbar::processEvent( Time_t time, Event* event  )
bool Xbar::processEvent( Event* event  )
{
    // KSH FIXME
//     _XBAR_DBG( "id=%lu time=%.9f cycle=%lu\n", Id(), 
//                 CurrentTime(), CurrentCycle() );
    _XBAR_DBG("got an event from the xbar @ cycle %ld\n", getCurrentSimTime() );
    cpu->Send( 3, static_cast<CompEvent*>(event) );
    return false;
}

bool Xbar::selfEvent( Event* event  )
{
    // KSH FIXME
//     _XBAR_DBG( "id=%lu time=%.9f cycle=%lu\n", Id(), 
//                 CurrentTime(), CurrentCycle() );
    _XBAR_DBG("got a pushed self event @ cycle %ld\n", getCurrentSimTime() );
    selfPush->Send( 50, static_cast<CompEvent*>(event) );
    return false;
}


extern "C" {
Xbar* xbarAllocComponent( SST::ComponentId_t id,
                          SST::Component::Params_t& params )
{
    return new Xbar( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(Xbar)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Xbar, bool, SST::Cycle_t, SST::Time_t )

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Xbar, bool, SST::Time_t, SST::Event* )
    
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Xbar, bool, SST::Cycle_t )

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Xbar, bool, SST::Event* )
#endif
