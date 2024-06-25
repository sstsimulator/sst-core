// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_PRIORITY_QUEUE_H
#define SST_CORE_SERIALIZATION_SERIALIZE_PRIORITY_QUEUE_H

#include "sst/core/serialization/serializer.h"

#include <queue>

namespace SST {
namespace Core {
namespace Serialization {


template <class T, class S, class C>
class serialize<std::priority_queue<T, S, C>>
{
    typedef std::priority_queue<T, S, C> Pqueue;

public:
    S& getContainer(std::priority_queue<T, S, C>& q)
    {
        struct UnderlyingContainer : std::priority_queue<T, S, C>
        {
            static S& getUnderlyingContainer(std::priority_queue<T, S, C>& q) { return q.*&UnderlyingContainer::c; }
        };
        return UnderlyingContainer::getUnderlyingContainer(q);
    }

    void operator()(Pqueue& v, serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            size_t size = v.size();
            ser.size(size);

            auto container = getContainer(v);
            for ( auto it = container.begin(); it != container.end(); ++it ) {
                T&   t = const_cast<T&>(*it);
                ser& t;
            }
            break;
        }
        case serializer::PACK:
        {
            size_t size = v.size();
            ser.pack(size);

            auto container = getContainer(v);
            for ( auto it = container.begin(); it != container.end(); ++it ) {
                T&   t = const_cast<T&>(*it);
                ser& t;
            }
            break;
        }
        case serializer::UNPACK:
        {
            size_t size;
            ser.unpack(size);
            for ( size_t i = 0; i < size; ++i ) {
                T    t = {};
                ser& t;
                v.push(t);
            }
            break;
        }
        }
    }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_PRIORITY_QUEUE_H
