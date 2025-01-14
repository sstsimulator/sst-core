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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_PRIORITY_QUEUE_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_PRIORITY_QUEUE_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_priority_queue.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

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
        case serializer::MAP:
            // The version of function not called in mapping mode
            break;
        }
    }

    void operator()(Pqueue& UNUSED(v), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Add support for mapping mode
    }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_PRIORITY_QUEUE_H
