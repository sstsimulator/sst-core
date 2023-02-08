// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_MEMPOOLTEST_H
#define SST_CORE_CORETEST_MEMPOOLTEST_H

#include "sst/core/component.h"
#include "sst/core/link.h"

#include <vector>

namespace SST {
namespace CoreTestMemPoolTest {


// We'll have 4 different sized events

class MemPoolTestEvent1 : public SST::Event
{
public:
    MemPoolTestEvent1() : SST::Event() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& array;
    }

    ImplementSerializable(SST::CoreTestMemPoolTest::MemPoolTestEvent1);

    std::string toString() const override
    {
        return std::string("MemPoolTestEvent1 to be delivered at ") + std::to_string(getDeliveryTime());
    }

private:
    uint64_t array[1];
};

class MemPoolTestEvent2 : public SST::Event
{
public:
    MemPoolTestEvent2() : SST::Event() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& array;
    }

    ImplementSerializable(SST::CoreTestMemPoolTest::MemPoolTestEvent2);

    std::string toString() const override
    {
        return std::string("MemPoolTestEvent2 to be delivered at ") + std::to_string(getDeliveryTime());
    }

private:
    uint64_t array[2];
};

class MemPoolTestEvent3 : public SST::Event
{
public:
    MemPoolTestEvent3() : SST::Event() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& array;
    }

    ImplementSerializable(SST::CoreTestMemPoolTest::MemPoolTestEvent3);

    std::string toString() const override
    {
        return std::string("MemPoolTestEvent3 to be delivered at ") + std::to_string(getDeliveryTime());
    }

private:
    uint64_t array[3];
};

class MemPoolTestEvent4 : public SST::Event
{
public:
    MemPoolTestEvent4() : SST::Event() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& array;
    }

    ImplementSerializable(SST::CoreTestMemPoolTest::MemPoolTestEvent4);

    std::string toString() const override
    {
        return std::string("MemPoolTestEvent4 to be delivered at ") + std::to_string(getDeliveryTime());
    }

private:
    uint64_t array[4];
};

class MemPoolTestPerformanceEvent : public SST::Event
{
public:
    MemPoolTestPerformanceEvent() : SST::Event() {}

    double rate;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& rate;
    }

    ImplementSerializable(SST::CoreTestMemPoolTest::MemPoolTestPerformanceEvent);
};


// Class will just send messages to other side of link.  They can be
// configured to send different size events to test the mempool
// overflow feature
class MemPoolTestComponent : public SST::Component
{
public:
    SST_ELI_REGISTER_COMPONENT(
        MemPoolTestComponent,
        "coreTestElement",
        "memPoolTestComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Test MemPool overflow",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "event_size", "Size of event to sent (valid sizes: 1-4).", "1" },
        { "initial_events", "Number of events to send to each other component", "256" },
        { "undeleted_events", "Number of events to leave undeleted", "0" },
        { "check_overflow", "Check to see whether MemPool overflow is working correctly", "true"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"port%d", "Links to other test components", { "CoreTestMemPoolTest.MemPoolTestEvent", "" } }
    )

    SST_ELI_DOCUMENT_ATTRIBUTES(
        { "test_element", "true" }
    )

    MemPoolTestComponent(ComponentId_t id, Params& params);
    ~MemPoolTestComponent() {}

    void eventHandler(Event* ev, int port);
    void setup(void) override;
    void finish(void) override;
    void complete(unsigned int phase) override;

private:
    int                event_size;
    std::vector<Link*> links;
    int                events_sent;
    int                events_recv;
    int                initial_events;
    double             event_rate;
    int                undeleted_events;
    bool               check_overflow;

    Event* createEvent();
};

} // namespace CoreTestMemPoolTest
} // namespace SST

#endif // SST_CORE_CORETEST_COMPONENT_H
