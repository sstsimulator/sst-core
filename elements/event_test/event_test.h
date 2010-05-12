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


#ifndef _EVENT_TEST_H
#define _EVENT_TEST_H

#include <sst/event.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/timeConverter.h>


class event_test : public SST::Component {
public:
    event_test( SST::ComponentId_t id, SST::Component::Params_t& params );

    int Setup();
    int Finish() {
	return 0;
    }
    
private:

    int my_id;
    int count_to;
    int latency;
    bool done;

    SST::Link* link;
    
    bool handleEvent( SST::Event *ev );

};

#endif
