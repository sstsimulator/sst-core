// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTERFACES_STRINGEVENT_H
#define SST_CORE_INTERFACES_STRINGEVENT_H

#include "sst/core/event.h"
#include "sst/core/sst_types.h"

namespace SST {
namespace Interfaces {

/**
 * Simple event to pass strings between components
 */
class StringEvent : public SST::Event, public SST::Core::Serialization::serializable_type<StringEvent>
{
public:
    StringEvent() {} // For serialization only

    /** Create a new StringEvent
     * @param str - The String contents of this event
     */
    StringEvent(const std::string& str) : SST::Event(), str(str) {}

    /** Clone a StringEvent */
    virtual Event* clone() override { return new StringEvent(*this); }

    /** Returns the contents of this Event */
    const std::string& getString(void) { return str; }

private:
    std::string str;

public:
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& str;
    }

    ImplementSerializable(SST::Interfaces::StringEvent);
};

} // namespace Interfaces
} // namespace SST

#endif // SST_CORE_INTERFACES_STRINGEVENT_H
