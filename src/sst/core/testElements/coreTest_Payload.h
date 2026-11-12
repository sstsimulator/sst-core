// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_PAYLOAD_H
#define SST_CORE_CORETEST_PAYLOAD_H

#include "sst/core/component.h"
#include "sst/core/event.h"
#include "sst/core/link.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace SST::CoreTestPayload {

/**
   Event with a payload whose contents are determined by its sequence number, so that the receiver can check every
   byte. The payload has a vector of words, which is serialized one element at a time, and a string, which is
   serialized as one large block.
*/
class coreTestPayloadEvent : public SST::Event
{
public:
    coreTestPayloadEvent() = default;
    coreTestPayloadEvent(uint64_t seq, size_t size);

    // Returns true if the payload has the expected size and contents for its sequence number
    bool check(size_t size) const;

    uint64_t seq = 0;
    size_t   bytes() const { return words.size() * sizeof(uint32_t) + str.size(); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        SST_SER(seq);
        SST_SER(words);
        SST_SER(str);
    }

    ImplementSerializable(SST::CoreTestPayload::coreTestPayloadEvent);

private:
    std::vector<uint32_t> words;
    std::string           str;
};

/**
   Component which exchanges events with large, varying payloads with its neighbors, and checks the payloads it
   receives. It is used to test the transfer of serialized data between ranks.
*/
class coreTestPayloadComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestPayloadComponent,
        "coreTestElement",
        "coreTestPayloadComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Exchanges events with large payloads with its neighbors and checks them",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "num_cycles",       "Number of clock cycles in which events are sent", "20" },
        { "events_per_cycle", "Number of events sent on each link each cycle", "4" },
        { "max_payload",      "Maximum payload size of an event, in words and in string bytes", "4096" },
        { "init_payload",     "Payload size of the event sent on each link during init", "1000" },
        { "padding",          "Unused. Makes the configuration graph larger, to test its distribution", "" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS()

    SST_ELI_DOCUMENT_PORTS(
        {"Elink", "Link to the East",  { "coreTestPayloadEvent", "" } },
        {"Wlink", "Link to the West",  { "coreTestPayloadEvent", "" } }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestPayloadComponent(SST::ComponentId_t id, SST::Params& params);
    ~coreTestPayloadComponent() = default;

    void init(unsigned int phase) override;
    void setup() override {}
    void finish() override;

private:
    struct link_info
    {
        std::string name;
        SST::Link*  link;
        uint64_t    events = 0;
        uint64_t    bytes  = 0;
        uint64_t    errors = 0;
        uint64_t    init   = 0;
    };

    uint64_t num_cycles;
    uint64_t events_per_cycle;
    uint64_t max_payload;
    uint64_t init_payload;
    uint64_t next_seq    = 1;
    uint64_t cycles_sent = 0;
    bool     sending     = true;
    bool     ended       = false;

    link_info links[2];

    // Payload size of the event with sequence number seq. Sizes vary so that the transfers between ranks grow and
    // shrink.
    size_t payloadSize(uint64_t seq) const;

    void handleEvent(SST::Event* ev, int index);
    bool clockTic(SST::Cycle_t);
    void checkDone();
};

} // namespace SST::CoreTestPayload

#endif // SST_CORE_CORETEST_PAYLOAD_H
