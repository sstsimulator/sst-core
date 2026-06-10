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

#ifndef SST_CORE_CORETEST_CLOCKERCOMPONENT_H
#define SST_CORE_CORETEST_CLOCKERCOMPONENT_H

#include "sst/core/component.h"

#include <cstdint>
#include <string>

namespace SST::CoreTestClockerComponent {

class coreTestClockerComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestClockerComponent,
        "coreTestElement",
        "coreTestClockerComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Clock Benchmark Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
        {"left", "Left port", { "" } },
        {"right", "Right port", { "" } },
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestClockerComponent(SST::ComponentId_t id, SST::Params& params);
    void setup() override;
    void finish() override {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(coreTestClockerComponent);

private:
    coreTestClockerComponent()                                           = default; // for serialization only
    ~coreTestClockerComponent()                                          = default;
    coreTestClockerComponent(const coreTestClockerComponent&)            = delete; // do not implement
    coreTestClockerComponent& operator=(const coreTestClockerComponent&) = delete; // do not implement

    // Variables to control the simulation
    static constexpr int            total_count   = 3;
    inline static const std::string master_period = "5ns";
    inline static const std::string test_period   = "1ns";

    TimeConverter test_tc;

    // My id is 0 if left_ == nullptr, otherwise, I get an event on my left port and get my id from there, then
    // increment it and send it on my right port, if I have one.
    int  id_         = -1;
    int  inst_count_ = 0;
    bool done_       = false;

    Link* left_  = nullptr;
    Link* right_ = nullptr;

    void handle_left(Event* ev);

    /*
      Handler for the "master" clock.  This will read the instructions and send an event on a self link to execute the
      instruction.  This is done because we can't guarantee the order of execution for the master and test clocks.
    */
    bool master_handler(Cycle_t cycle);

    /*
      Self link to send instructions on
    */
    Link* inst_link_;

    /*
      Handler for the instruction self link
    */
    void inst_handler(Event* ev);

    /*
      Handler for the "tsst" clocks
    */
    bool test_handler(Cycle_t cycle, int clock_index);

    /*
      There are multiple clocks to test out the various clock APIs in the core:

      Master clock:
        Period: 50ns
        - This clock will follow a schedule and start/stop all the other clocks in the simulation

      ** Clocks using the old API
      Clock 0:
        Period: 1ns
        - This clock will count X cycles, then remove itself by returning false from the handler.
          The master clock will restart it using reregisterClock().

      Clock 1:
        Period: 1ns
        - This clock will run until removed from the clock list by the master clock using unregsterClock().
          The master clock will then restart it later using reregisterClock().

      ** Clocks using the new API
      Clock 2:
        Period: 1ns
        - This clock will count X cycles, then remove itself by returning false from the handler.
          The master clock will restart it using activate().

      Clock 3:
        Period: 1ns
        - This clock will run until removed from the clock list by the master clock using deactivate(),
          The master clock will then restart it later using activate().
     */

    // Struct to hold the data needed to manage the clocks
    struct ClockInfo
    {
        Clock::HandlerBase* handler = nullptr;
        TimeConverter       period;
        int64_t             counter   = 0;
        bool                new_style = false;

        void serialize_order(SST::Core::Serialization::serializer& ser)
        {
            SST_SER(handler);
            SST_SER(period);
            SST_SER(counter);
            SST_SER(new_style);
        }
    };

    // Handler for master clock
    Clock::HandlerBase* master_ = nullptr;

    // Data for the clocks being tested
    std::vector<ClockInfo> clocks_;

private:

    // Operations that the master handler can peform
    enum class Op { nop, start, stop, term };

    struct OpBundle
    {
        Op  op    = Op::nop;
        int clock = -1;
    };

    // Everyone does the same program, so make it static
    inline static const std::vector<OpBundle> instructions = { { Op::start, 1 }, { Op::start, 0 }, { Op::start, 3 },
        { Op::stop, 1 }, { Op::start, 2 }, { Op::stop, 3 }, { Op::start, 1 }, { Op::start, 0 }, { Op::start, 3 },
        { Op::stop, 1 }, { Op::start, 2 }, { Op::stop, 3 }, { Op::term, -1 } };

    using OpEvent  = BasicEvent<OpBundle>;
    using IntEvent = BasicEvent<int>;
};

} // namespace SST::CoreTestClockerComponent

#endif // SST_CORE_CORETEST_CLOCKERCOMPONENT_H
