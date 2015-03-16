// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "TestEvent.h"

#include "sst/core/element.h"


using namespace SST;
using namespace SST::Interfaces;

// Yes, this is trivially easy and could be inlined, but it's useful to make sure
// the requireEvent code works properly. 
TestEvent::TestEvent() : SST::Event(), print_on_delete(false)
{
}

TestEvent::~TestEvent()
{
    if ( print_on_delete ) {
	printf("Deleting TestEvent\n");
    }
}

BOOST_CLASS_EXPORT(TestEvent)
