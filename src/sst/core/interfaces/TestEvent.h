// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef INTERFACES_TEST_EVENT_H
#define INTERFACES_TEST_EVENT_H

#include <sst/core/event.h>

namespace SST{
namespace Interfaces {

/**  Test Event
 *   Useful for early-testing of components.
 */
class TestEvent : public SST::Event, public SST::Core::Serialization::serializable_type<TestEvent> {
public:
    TestEvent();
    ~TestEvent();
    /** Unused */
    int count;
    /** Prints a message to stdout when the message is deleted. */
    bool print_on_delete;

public:   
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Event::serialize_order(ser);
        ser & count;
    }
    
    ImplementSerializable(SST::Interfaces::TestEvent);     
};

} //namespace Interfaces
} //namespace SST

#endif
