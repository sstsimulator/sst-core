// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/pollingLinkQueue.h"

namespace SST {

PollingLinkQueue::~PollingLinkQueue()
{
    // Need to delete any events left in the queue
    for ( auto it = data.begin(); it != data.end(); ++it ) {
        delete *it;
    }
    data.clear();
}

bool
PollingLinkQueue::empty()
{
    return data.empty();
}

int
PollingLinkQueue::size()
{
    return data.size();
}

void
PollingLinkQueue::insert(Activity* activity)
{
    data.insert(activity);
}

Activity*
PollingLinkQueue::pop()
{
    if ( data.size() == 0 ) return nullptr;
    auto      it      = data.begin();
    Activity* ret_val = (*it);
    data.erase(it);
    return ret_val;
}

Activity*
PollingLinkQueue::front()
{
    if ( data.size() == 0 ) return nullptr;
    return *data.begin();
}

void
PollingLinkQueue::serialize_order(SST::Core::Serialization::serializer& ser)
{
    switch ( ser.mode() ) {
    case SST::Core::Serialization::serializer::SIZER:
    case SST::Core::Serialization::serializer::PACK:
    {
        size_t size = data.size();
        SST_SER(size);
        for ( auto* x : data ) {
            SST_SER(x);
        }
        break;
    }
    case SST::Core::Serialization::serializer::UNPACK:
    {
        size_t size;
        SST_SER(size);
        Activity* activity;
        for ( size_t i = 0; i < size; ++i ) {
            SST_SER(activity);
            data.insert(activity);
        }
    }
    case SST::Core::Serialization::serializer::MAP:
        // This version of function not called in mapping mode
        break;
    }
}

} // namespace SST
