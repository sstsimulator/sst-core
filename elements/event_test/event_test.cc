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

#include "event_test.h"
#include "myEvent.h"

using namespace SST;
// BOOST_CLASS_EXPORT( SST::MyEvent )

event_test::event_test( ComponentId_t id, Params_t& params ) :
    Component( id )
{

    if ( params.find("id") == params.end() ) {
        _abort(event_test,"couldn't find node id\n");
    }
    my_id = strtol( params[ "id" ].c_str(), NULL, 0 );
    

    if ( params.find("count_to") == params.end() ) {
        _abort(event_test,"couldn't find count_to\n");
    }
    count_to = strtol( params[ "count_to" ].c_str(), NULL, 0 );


    if ( params.find("latency") == params.end() ) {
        _abort(event_test,"couldn't find latency\n");
    }
    latency = strtol( params[ "latency" ].c_str(), NULL, 0 );
    
    registerExit();
    done = false;

    Event::Handler_t* linkHandler = new EventHandler<event_test,bool,Event*>
	(this,&event_test::handleEvent);

    link = LinkAdd( "link", linkHandler );
    registerTimeBase("1ns");
    
}

int event_test::Setup() {
    if ( my_id == 0 ) {
	MyEvent* event = new MyEvent();
	event->count = 0;
	link->Send(latency,event);
	printf("Sending initial event\n");
    }
    else if ( my_id != 1) {
        _abort(event_test,"event_test class only works with two instances\n");	
    }
    return 0;
}

bool event_test::handleEvent(Event* ev) {
    MyEvent* event = static_cast<MyEvent*>(ev);
    // See if we have counted far enough, if not, increment and send
    // back
    if (event->count > count_to) {
	event->count++;
	link->Send(latency,event);
	if (!done) {
	    unregisterExit();
	    done = true;
	}
    }
    else {
	printf("%d: %d\n",my_id,event->count);
	event->count++;
	link->Send(latency,event);
    }    
    return false;
}
    
extern "C" {
event_test*
event_testAllocComponent( SST::ComponentId_t id, 
			  SST::Component::Params_t& params )
{
    return new event_test( id, params );
}
}
